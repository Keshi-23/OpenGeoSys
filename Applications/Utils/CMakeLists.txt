set(CMAKE_FOLDER "Utilities")
add_subdirectory(FileConverter)
add_subdirectory(GeoTools)
add_subdirectory(MeshGeoTools)
add_subdirectory(MeshEdit)
add_subdirectory(ModelPreparation)
add_subdirectory(PostProcessing)
add_subdirectory(SimpleMeshCreation)

if(OGS_BUILD_GUI)
    add_subdirectory(OGSFileConverter)
endif()

if(OGS_BUILD_SWMM)
    add_subdirectory(SWMMConverter)
endif()

if(OGS_BUILD_TESTING)
    include(Tests.cmake)
endif()

unset(CMAKE_FOLDER)
