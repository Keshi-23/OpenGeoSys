# Returns a list of source files (*.h and *.cpp) in source_files and creates a
# Visual Studio folder. A (relative) subdirectory can be passed as second
# parameter (optional).
macro(GET_SOURCE_FILES source_files)
    if(${ARGC} EQUAL 2)
        set(DIR "${ARGV1}")
    else()
        set(DIR ".")
    endif()

    # Get all files in the directory
    file(GLOB GET_SOURCE_FILES_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
         CONFIGURE_DEPENDS ${DIR}/*.h
    )
    file(GLOB GET_SOURCE_FILES_TEMPLATES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
         CONFIGURE_DEPENDS ${DIR}/*.tpp
    )
    file(GLOB GET_SOURCE_FILES_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
         CONFIGURE_DEPENDS ${DIR}/*.cpp
    )

    set(${source_files}
        ${GET_SOURCE_FILES_HEADERS} ${GET_SOURCE_FILES_TEMPLATES}
        ${GET_SOURCE_FILES_SOURCES}
    )
    list(LENGTH ${source_files} NUM_FILES)
    if(${NUM_FILES} EQUAL 0)
        message(FATAL_ERROR "No source files found in ${DIR}")
    endif()
endmacro()

# Appends a list of source files (*.h and *.cpp) to source_files and creates a
# Visual Studio folder. A (relative) subdirectory can be passed as second
# parameter (optional).
macro(APPEND_SOURCE_FILES source_files)
    if(${ARGC} EQUAL 2)
        set(DIR "${ARGV1}")
    else()
        set(DIR ".")
    endif()

    GET_SOURCE_FILES(TMP_SOURCES "${DIR}")
    set(${source_files} ${${source_files}} ${TMP_SOURCES})
endmacro()

# Creates one ctest for each googletest found in source files passed as
# arguments number two onwards. Argument one specifies the testrunner
# executable.
macro(ADD_GOOGLE_TESTS executable)
    foreach(source ${ARGN})
        file(READ "${source}" contents)
        string(REGEX MATCHALL "TEST_?F?\\(([A-Za-z_0-9 ,]+)\\)" found_tests
                     ${contents}
        )
        foreach(hit ${found_tests})
            string(REGEX REPLACE ".*\\(([A-Za-z_0-9]+)[, ]*([A-Za-z_0-9]+)\\).*"
                                 "\\1.\\2" test_name ${hit}
            )
            add_test(${test_name} ${executable} --gtest_output=xml
                     --gtest_filter=${test_name} ${MI3CTestingDir}
            )
        endforeach()
    endforeach()
endmacro()

# Adds the include dir containing the autogenerated files to the PUBLIC
# interface of the given target
function(add_autogen_include target)
    get_property(IsMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(IsMultiConfig)
        target_include_directories(
            ${target}
            PUBLIC
                ${CMAKE_CURRENT_BINARY_DIR}/${target}_autogen/include_$<CONFIG>
        )
    else()
        target_include_directories(
            ${target}
            PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/${target}_autogen/include
        )
    endif()
endfunction()

# Replacement for add_library() for ogs targets
function(ogs_add_library targetName)
    foreach(file ${ARGN})
        # cmake-lint: disable=E1126
        file(REAL_PATH ${file} file_path)
        list(APPEND files ${file_path})
    endforeach()

    add_library(${targetName} ${files})
    target_compile_options(
        ${targetName}
        PRIVATE # OR does not work with cotire
                # $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,
                # $<CXX_COMPILER_ID:GNU>>:-Wall -Wextra>
                $<$<CXX_COMPILER_ID:Clang>:-Wall
                -Wextra
                -Wunreachable-code>
                $<$<CXX_COMPILER_ID:AppleClang>:-Wall
                -Wextra
                -Wunreachable-code>
                $<$<CXX_COMPILER_ID:GNU>:-Wall
                -Wextra
                -Wunreachable-code>
                $<$<CXX_COMPILER_ID:MSVC>:/W3>
    )

    if(BUILD_SHARED_LIBS)
        install(TARGETS ${targetName}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()

    include(GenerateExportHeader)
    generate_export_header(${targetName})
    target_include_directories(${targetName} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
        set_target_properties(
            ${targetName} PROPERTIES UNITY_BUILD ${OGS_USE_UNITY_BUILDS}
        )
    endif()
    GroupSourcesByFolder(${targetName})
endfunction()

# Parses current directory into a list
function(current_dir_as_list baseDir outList)
    file(RELATIVE_PATH REL_DIR ${PROJECT_SOURCE_DIR}/${baseDir}
         ${CMAKE_CURRENT_LIST_DIR}
    )
    string(REPLACE "/" ";" DIR_LIST ${REL_DIR})
    set(${outList} ${DIR_LIST} PARENT_SCOPE)
endfunction()
