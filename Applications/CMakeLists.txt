add_subdirectory(ApplicationsLib)
add_subdirectory(DataHolderLib)
add_subdirectory(FileIO)

if(OGS_BUILD_UTILS AND NOT _IS_SUBPROJECT)
    add_subdirectory(Utils)
elseif(OGS_BUILD_GUI)
    add_subdirectory(Utils/OGSFileConverter)
endif() # OGS_BUILD_UTILS AND NOT _IS_SUBPROJECT

if(OGS_BUILD_GUI)
    add_subdirectory(DataExplorer)
endif() # OGS_BUILD_GUI

if(OGS_BUILD_CLI)
    add_subdirectory(CLI)
endif() # OGS_BUILD_CLI

if(OGS_USE_INSITU)
    add_subdirectory(InSituLib)
endif()

if(OGS_BUILD_WHEEL)
    add_subdirectory(Python)
endif()
