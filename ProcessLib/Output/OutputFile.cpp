/**
 * \file
 * \copyright
 * Copyright (c) 2012-2022, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include "OutputFile.h"

#include <cassert>
#include <exception>
#include <fstream>
#include <vector>

#include "BaseLib/DisableFPE.h"
#include "BaseLib/FileTools.h"
#include "MeshLib/IO/VtkIO/PVDFile.h"
#include "MeshLib/IO/VtkIO/VtuInterface.h"
#include "MeshLib/IO/XDMF/XdmfHdfWriter.h"
#include "ProcessLib/Process.h"

namespace ProcessLib
{
/**
 * Get a reference to the PVDFile corresponding to the given filename
 * @param mesh_name the name of the mesh the PVD file is searched for
 * @param mesh_name_to_pvd_file the name to PVD objects mapping
 * @return Reference to a PVDFile object
 */
MeshLib::IO::PVDFile& findPVDFile(
    std::string const& mesh_name,
    std::map<std::string, MeshLib::IO::PVDFile> const& mesh_name_to_pvd_file)
{
    auto const it = mesh_name_to_pvd_file.find(mesh_name);
    if (it == mesh_name_to_pvd_file.end())
    {
        OGS_FATAL(
            "The given mesh is not contained in the output configuration. "
            "Aborting.");
    }

    return const_cast<MeshLib::IO::PVDFile&>(it->second);
}

void outputMeshVtk(std::string const& file_name, MeshLib::Mesh const& mesh,
                   bool const compress_output, int const data_mode)
{
    DBUG("Writing output to '{:s}'.", file_name);

    MeshLib::IO::VtuInterface vtu_interface(&mesh, data_mode, compress_output);
    vtu_interface.writeToFile(file_name);
}

void outputMeshVtk(OutputVTKFormat const& output_file,
                   MeshLib::IO::PVDFile& pvd_file, MeshLib::Mesh const& mesh,
                   double const t, int const timestep, int const iteration)
{
    auto const name =
        output_file.constructFilename(mesh.getName(), timestep, t, iteration);
    pvd_file.addVTUFile(name, t);

    auto const path = BaseLib::joinPaths(output_file.directory, name);
    // Output of NaN's triggers floating point exceptions. Because we are not
    // debugging VTK (or other libraries) at this point, the possibly set
    // exceptions are temporarily disabled and are restored at the end of the
    // function.
    [[maybe_unused]] DisableFPE disable_fpe;
    outputMeshVtk(path, mesh, output_file.compression, output_file.data_mode);
}

std::string OutputVTKFormat::constructPVDName(
    std::string const& mesh_name) const
{
    return BaseLib::joinPaths(
        directory,
        BaseLib::constructFormattedFileName(prefix, mesh_name, 0, 0, 0) +
            ".pvd");
}

OutputFile::OutputFile(std::string const& directory, std::string prefix,
                       std::string suffix, bool const compression)
    : directory(directory),
      prefix(std::move(prefix)),
      suffix(std::move(suffix)),
      compression(compression)
{
}

std::string OutputVTKFormat::constructFilename(std::string const& mesh_name,
                                               int const timestep,
                                               double const t,
                                               int const iteration) const
{
    return BaseLib::constructFormattedFileName(prefix, mesh_name, timestep, t,
                                               iteration) +
           BaseLib::constructFormattedFileName(suffix, mesh_name, timestep, t,
                                               iteration) +
           ".vtu";
}

std::string OutputXDMFHDF5Format::constructFilename(
    std::string const& mesh_name,
    int const timestep,
    double const t,
    int const iteration) const
{
    return BaseLib::constructFormattedFileName(prefix, mesh_name, timestep, t,
                                               iteration) +
           ".xdmf";
}

void OutputXDMFHDF5Format::outputMeshXdmf(
    std::set<std::string> const& output_variables,
    std::vector<std::reference_wrapper<const MeshLib::Mesh>> meshes,
    int const timestep, double const t, int const iteration) const
{
    // \TODO The XdmfOutput will create on construction the XdmfHdfWriter
    if (!mesh_xdmf_hdf_writer)
    {
        auto name = constructFilename(meshes[0].get().getName(), timestep, t,
                                      iteration);
        std::filesystem::path path(BaseLib::joinPaths(directory, name));
        mesh_xdmf_hdf_writer = std::make_unique<MeshLib::IO::XdmfHdfWriter>(
            std::move(meshes), path, timestep, t, output_variables, compression,
            n_files);
    }
    else
    {
        mesh_xdmf_hdf_writer->writeStep(t);
    };
}

void OutputVTKFormat::outputMeshes(
    const int timestep, const double t, const int iteration,
    std::vector<std::reference_wrapper<const MeshLib::Mesh>> const& meshes,
    [[maybe_unused]] std::set<std::string> const& output_variables) const
{
    for (auto const& mesh : meshes)
    {
        auto& pvd_file =
            findPVDFile(mesh.get().getName(), mesh_name_to_pvd_file);
        outputMeshVtk(*this, pvd_file, mesh, t, timestep, iteration);
    }
}

void OutputVTKFormat::addProcess(
    std::vector<std::string> const& mesh_names_for_output)
{
    for (auto const& mesh_output_name : mesh_names_for_output)
    {
        auto const filename = constructPVDName(mesh_output_name);
        mesh_name_to_pvd_file.emplace(std::piecewise_construct,
                                      std::forward_as_tuple(mesh_output_name),
                                      std::forward_as_tuple(filename));
    }
}
}  // namespace ProcessLib
