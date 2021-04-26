/**
 * \file
 * \copyright
 * Copyright (c) 2012-2021, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 */

#include "writeMeshToFile.h"

#include "BaseLib/FileTools.h"
#include "BaseLib/Logging.h"
#include "BaseLib/StringTools.h"
#include "MeshLib/IO/Legacy/MeshIO.h"
#include "MeshLib/IO/VtkIO/VtuInterface.h"
#ifdef OGS_USE_XDMF
#include "MeshLib/IO/XDMF/XdmfHdfWriter.h"
#endif
#include "MeshLib/Mesh.h"

namespace MeshLib::IO
{
int writeMeshToFile(const MeshLib::Mesh& mesh,
                    std::filesystem::path const& file_path,
                    std::set<std::string>
                        variable_output_names)
{
    if (file_path.extension().string() == ".msh")
    {
        MeshLib::IO::Legacy::MeshIO meshIO;
        meshIO.setMesh(&mesh);
        BaseLib::IO::writeStringToFile(meshIO.writeToString(), file_path);
        return 0;
    }
    if (file_path.extension().string() == ".vtu")
    {
        MeshLib::IO::VtuInterface writer(&mesh);
        auto const result = writer.writeToFile(file_path);
        if (!result)
        {
            ERR("writeMeshToFile(): Could not write mesh to '{:s}'.",
                file_path.string());
            return -1;
        }
        return 0;
    }
#ifdef OGS_USE_XDMF
    if (file_path.extension().string() == ".xdmf")
    {
        MeshLib::IO::XdmfHdfWriter(
            mesh, file_path, 0, variable_output_names, true);

        return 0;
    }
#endif
    ERR("writeMeshToFile(): Unknown mesh file format in file {:s}.",
        file_path.string());
    return -1;
}

}  // namespace MeshLib::IO
