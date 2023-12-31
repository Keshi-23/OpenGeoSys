set(TOOLS
    AssignRasterDataToMesh
    computeSurfaceNodeIDsInPolygonalRegion
    constructMeshesFromGeometry
    createIntermediateRasters
    identifySubdomains
    IntegrateBoreholesIntoMesh
    Raster2Mesh
    geometryToGmshGeo
)

if(OGS_BUILD_GUI)
    # Utility requires that GUI is build because it uses Qt-based functionality
    # Also requires gmsh at runtime
    list(APPEND TOOLS VerticalSliceFromLayers)
endif()

foreach(tool ${TOOLS})
    ogs_add_executable(${tool} ${tool}.cpp)
    target_link_libraries(
        ${tool} ApplicationsFileIO GitInfoLib MeshLib MeshToolsLib MeshGeoToolsLib tclap
    )
endforeach()

if(TARGET VerticalSliceFromLayers)
    target_link_libraries(VerticalSliceFromLayers Qt5::Xml Qt5::XmlPatterns)
endif()

install(TARGETS ${TOOLS} RUNTIME DESTINATION bin)
