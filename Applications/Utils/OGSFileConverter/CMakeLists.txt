ogs_add_library(
    OGSFileConverterLib FileListDialog.h OGSFileConverter.h FileListDialog.cpp
    OGSFileConverter.cpp
)
target_link_libraries(
    OGSFileConverterLib PUBLIC ApplicationsFileIO MathLib QtBase
    INTERFACE MeshLib MeshToolsLib
)

set_target_properties(OGSFileConverterLib PROPERTIES AUTOMOC TRUE AUTOUIC TRUE)

ogs_add_executable(OGSFileConverter main.cpp)

target_link_libraries(
    OGSFileConverter PUBLIC ApplicationsFileIO GitInfoLib OGSFileConverterLib
                            tclap
)

add_autogen_include(OGSFileConverterLib)

install(TARGETS OGSFileConverter RUNTIME DESTINATION bin)
