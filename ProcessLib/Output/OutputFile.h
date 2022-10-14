/**
 * \file
 * \copyright
 * Copyright (c) 2012-2022, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#pragma once

#include <iosfwd>
#include <vector>

#include "MeshLib/IO/VtkIO/PVDFile.h"
#include "MeshLib/IO/XDMF/XdmfHdfWriter.h"

namespace ProcessLib
{
class Process;

struct OutputFile
{
    OutputFile(std::string const& directory, std::string prefix,
               std::string suffix, bool const compression);
    virtual ~OutputFile() = default;

    OutputFile(OutputFile const& other) = delete;
    OutputFile(OutputFile&& other) = default;
    OutputFile& operator=(OutputFile& other) = delete;
    OutputFile& operator=(OutputFile&& other) = default;

    std::string directory;
    std::string prefix;
    std::string suffix;
    //! Enables or disables zlib-compression of the output files.
    bool compression;

    virtual void outputMeshes(
        const Process& process, const int process_id, const int timestep,
        const double t, const int iteration,
        std::vector<std::reference_wrapper<const MeshLib::Mesh>> const& meshes,
        std::set<std::string> const& output_variables) const = 0;

    virtual void addProcess(
        [[maybe_unused]] ProcessLib::Process const& process,
        [[maybe_unused]] std::vector<std::string> const& mesh_names_for_output)
    {
    }

    virtual std::string constructFilename(
        [[maybe_unused]] std::string const& mesh_name,
        [[maybe_unused]] int const timestep, [[maybe_unused]] double const t,
        [[maybe_unused]] int const iteration) const
    {
        return "";
    }
};

inline std::ostream& operator<<(std::ostream& os, OutputFile const& of)
{
    os << "OutputFile::directory:" << of.directory << std::endl;
    os << "OutputFile::prefix:" << of.prefix << std::endl;
    os << "OutputFile::suffix:" << of.suffix << std::endl;
    os << "OutputFile::compression:" << of.compression << std::endl;
    return os;
}

struct OutputVTKFormat final : public OutputFile
{
    OutputVTKFormat(std::string const& directory, std::string prefix,
                    std::string suffix, bool const compression,
                    int const data_mode)
        : OutputFile(directory, std::move(prefix), std::move(suffix),
                     compression),
          data_mode(data_mode)
    {
    }

    void outputMeshes(
        const Process& process, const int process_id, const int timestep,
        const double t, const int iteration,
        std::vector<std::reference_wrapper<const MeshLib::Mesh>> const& meshes,
        std::set<std::string> const& output_variables) const override;

    void addProcess(
        Process const& process,
        std::vector<std::string> const& mesh_names_for_output) override;

    //! Chooses vtk's data mode for output following the enumeration given
    /// in the vtkXMLWriter: {Ascii, Binary, Appended}.  See vtkXMLWriter
    /// documentation
    /// http://www.vtk.org/doc/nightly/html/classvtkXMLWriter.html
    int data_mode;

    //! Holds the PVD files associated with each process.
    //!
    //! Each \c process_id of each Process (in the current simulation) has a PVD
    //! file in this map for each element of Output::_mesh_names_for_output.
    //! I.e., the number of elements in this map is (roughly): <no. of
    //! processes> x <no. of process IDs per process> x <no. of meshes>.
    std::multimap<Process const*, MeshLib::IO::PVDFile> process_to_pvd_file;

    std::string constructFilename(std::string const& mesh_name,
                                  int const timestep, double const t,
                                  int const iteration) const override;

    std::string constructPVDName(std::string const& mesh_name) const;
};

struct OutputXDMFHDF5Format final : public OutputFile
{
    OutputXDMFHDF5Format(std::string const& directory, std::string prefix,
                         std::string suffix, bool const compression,
                         unsigned int const n_files)
        : OutputFile(directory, std::move(prefix), std::move(suffix),
                     compression),
          n_files(n_files)
    {
    }

    void outputMeshes(
        [[maybe_unused]] const Process& process,
        [[maybe_unused]] const int process_id, const int timestep,
        const double t, const int iteration,
        std::vector<std::reference_wrapper<const MeshLib::Mesh>> const& meshes,
        std::set<std::string> const& output_variables) const override
    {
        outputMeshXdmf(output_variables, std::move(meshes), timestep, t,
                       iteration);
    }

    std::string constructFilename(std::string const& mesh_name,
                                  int const timestep, double const t,
                                  int const iteration) const override;

    mutable std::unique_ptr<MeshLib::IO::XdmfHdfWriter> mesh_xdmf_hdf_writer;
    //! Specifies the number of hdf5 output files.
    unsigned int n_files;

    void outputMeshXdmf(
        std::set<std::string> const& output_variables,
        std::vector<std::reference_wrapper<const MeshLib::Mesh>> meshes,
        int const timestep, double const t, int const iteration) const;
};

void outputMeshVtk(std::string const& file_name, MeshLib::Mesh const& mesh,
                   bool const compress_output, int const data_mode);
}  // namespace ProcessLib
