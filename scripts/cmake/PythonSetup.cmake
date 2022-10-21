# cmake-lint: disable=C0103

if(OGS_USE_PYTHON)
    set(_python_version_max "...<3.12")
endif()

if(OGS_USE_PIP)
    set(_python_version_max "...<3.11") # There are no VTK wheels for >3.10
    set(Python_ROOT_DIR ${PROJECT_BINARY_DIR}/.venv)
    set(CMAKE_REQUIRE_FIND_PACKAGE_Python TRUE)
    if(NOT EXISTS ${PROJECT_BINARY_DIR}/.venv)
        execute_process(
            COMMAND
                ${CMAKE_COMMAND} -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
                -Dpython_version=${ogs.minimum_version.python}${_python_version_max}
                -P
                ${PROJECT_SOURCE_DIR}/scripts/cmake/PythonCreateVirtualEnv.cmake
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR} COMMAND_ECHO STDOUT
                              ECHO_OUTPUT_VARIABLE ECHO_ERROR_VARIABLE
        )
        unset(_OGS_PYTHON_PACKAGES_SHA1 CACHE)
    endif()
    set(_venv_bin_dir "bin")
    if(MSVC)
        set(_venv_bin_dir "Scripts")
    endif()
    set(LOCAL_VIRTUALENV_BIN_DIR ${PROJECT_BINARY_DIR}/.venv/${_venv_bin_dir}
        CACHE INTERNAL ""
    )
    # Fixes macOS install issues
    execute_process(
        COMMAND ${LOCAL_VIRTUALENV_BIN_DIR}/pip install wheel
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
else()
    # Prefer unix location over frameworks (Apple-only)
    set(Python_FIND_FRAMEWORK LAST)

    # Prefer more recent Python version
    set(Python_FIND_STRATEGY VERSION)
endif()

set(_python_componets Interpreter)
if(OGS_USE_PYTHON AND NOT OGS_BUILD_WHEEL)
    list(APPEND _python_componets Development.Embed)
endif()
if(OGS_BUILD_PYTHON_MODULE OR OGS_USE_PYTHON)
    list(APPEND _python_componets Development.Module)
endif()
if(OGS_USE_PYTHON OR OGS_BUILD_PYTHON_MODULE)
    set(CMAKE_REQUIRE_FIND_PACKAGE_Python TRUE)
endif()

find_package(
    Python ${ogs.minimum_version.python}${_python_version_max}
    COMPONENTS ${_python_componets}
)

if(OGS_USE_PIP)
    set(Python_SITEARCH_NATIVE ${Python_SITEARCH})
    if(WIN32)
        string(REPLACE "\\" "\\\\" Python_SITEARCH_NATIVE
                       ${Python_SITEARCH_NATIVE}
        )
    endif()
    set(OGS_PYTHON_PACKAGES ""
        CACHE INTERNAL "List of Python packages to be installed via pip."
    )
    set(Python_ROOT_DIR ${PROJECT_BINARY_DIR}/.venv)
    if(MSVC)
        set(Python_EXECUTABLE ${Python_ROOT_DIR}/Scripts/python.exe)
    else()
        set(Python_EXECUTABLE ${Python_ROOT_DIR}/bin/python)
    endif()
    if(OGS_BUILD_TESTING)
        # Notebook requirements from Tests/Data
        file(STRINGS Tests/Data/requirements.txt _requirements)
        file(STRINGS Tests/Data/requirements-dev.txt _requirements_dev)
        list(APPEND OGS_PYTHON_PACKAGES ${_requirements} ${_requirements_dev})

        list(APPEND OGS_PYTHON_PACKAGES
             "snakemake==${ogs.minimum_version.snakemake}"
        )
        set(SNAKEMAKE ${LOCAL_VIRTUALENV_BIN_DIR}/snakemake CACHE FILEPATH ""
                                                                  FORCE
        )
    endif()
endif()
