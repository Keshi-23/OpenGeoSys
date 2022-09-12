# cmake-lint: disable=C0103
message(STATUS "┌─ PythonCreateVirtualEnv.cmake")
list(APPEND CMAKE_MESSAGE_INDENT "│    ")

# Prefer unix location over frameworks (Apple-only)
set(Python_FIND_FRAMEWORK LAST)

# Prefer more recent Python version
set(Python_FIND_STRATEGY VERSION)

find_package(Python ${python_version} COMPONENTS Interpreter REQUIRED)

if(${Python_VERSION} VERSION_GREATER_EQUAL 3.9)
    set(_upgrade_deps --upgrade-deps)
endif()

execute_process(
    COMMAND ${Python_EXECUTABLE} -m venv ${_upgrade_deps} .venv
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(STATUS "└─ End PythonCreateVirtualEnv.cmake")
