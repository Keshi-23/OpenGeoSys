/**
 * \file
 * \copyright
 * Copyright (c) 2012-2022, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include "Output.h"

#include <cassert>
#include <exception>
#include <fstream>
#include <vector>

#include "AddProcessDataToMesh.h"
#include "Applications/InSituLib/Adaptor.h"
#include "BaseLib/FileTools.h"
#include "BaseLib/Logging.h"
#include "BaseLib/RunTime.h"
#include "ProcessLib/Process.h"

namespace ProcessLib
{
void addBulkMeshNodePropertyToSubMesh(MeshLib::Mesh const& bulk_mesh,
                                      MeshLib::Mesh& sub_mesh,
                                      std::string const& property_name)
{
    if (bulk_mesh == sub_mesh)
    {
        return;
    }
    if (!bulk_mesh.getProperties().existsPropertyVector<double>(
            property_name, MeshLib::MeshItemType::Node, 1))
    {
        return;
    }
    if (!sub_mesh.getProperties().existsPropertyVector<std::size_t>(
            "bulk_node_ids", MeshLib::MeshItemType::Node, 1))
    {
        return;
    }

    auto const& bulk_mesh_property =
        *bulk_mesh.getProperties().getPropertyVector<double>(property_name);
    auto const& bulk_node_ids =
        *sub_mesh.getProperties().getPropertyVector<std::size_t>(
            "bulk_node_ids");

    auto& sub_mesh_property = *MeshLib::getOrCreateMeshProperty<double>(
        sub_mesh, property_name, MeshLib::MeshItemType::Node, 1);

    std::transform(std::begin(bulk_node_ids), std::end(bulk_node_ids),
                   std::begin(sub_mesh_property),
                   [&bulk_mesh_property](auto const id)
                   { return bulk_mesh_property[id]; });
}

bool Output::isOutputProcess(const int process_id, const Process& process) const
{
    auto const is_last_process =
        process_id == static_cast<int>(_output_processes.size()) - 1;

    return process.isMonolithicSchemeUsed()
           // For the staggered scheme for the coupling, only the last
           // process, which gives the latest solution within a coupling
           // loop, is allowed to make output.
           || is_last_process;
}

Output::Output(std::unique_ptr<OutputFormat> output_format,
               bool const output_nonlinear_iteration_results,
               OutputDataSpecification const& output_data_specification,
               std::vector<std::string> const& mesh_names_for_output,
               std::vector<std::unique_ptr<MeshLib::Mesh>> const& meshes)
    : _output_format(std::move(output_format)),
      _output_nonlinear_iteration_results(output_nonlinear_iteration_results),
      _output_data_specification(std::move(output_data_specification)),
      _mesh_names_for_output(mesh_names_for_output),
      _meshes(meshes)
{
}

void Output::addProcess(ProcessLib::Process const& process)
{
    _output_processes.push_back(process);
    if (_mesh_names_for_output.empty())
    {
        _mesh_names_for_output.push_back(process.getMesh().getName());
    }
}

void Output::outputMeshes(
    const int timestep, const double t, const int iteration,
    std::vector<std::reference_wrapper<const MeshLib::Mesh>> const& meshes)
    const
{
    _output_format->outputMeshes(timestep, t, iteration, meshes,
                                 _output_data_specification.output_variables);
}

MeshLib::Mesh const& Output::prepareSubmesh(
    std::string const& submesh_output_name, Process const& process,
    const int process_id, double const t,
    std::vector<GlobalVector*> const& xs) const
{
    auto& submesh = *BaseLib::findElementOrError(
        _meshes.get().begin(), _meshes.get().end(),
        [&submesh_output_name](auto const& m)
        { return m->getName() == submesh_output_name; },
        "Need mesh '" + submesh_output_name + "' for the output.");

    DBUG("Found {:d} nodes for output at mesh '{:s}'.",
         submesh.getNumberOfNodes(), submesh.getName());

    bool const output_secondary_variables = false;

    // TODO Under the assumption that xs.size() and submesh do not change during
    // the simulation, process output data should not be recreated every time,
    // but should rather be computed only once and stored for later reuse.
    auto const process_output_data =
        createProcessOutputData(process, xs.size(), submesh);

    addProcessDataToMesh(t, xs, process_id, process_output_data,
                         output_secondary_variables,
                         _output_data_specification);

    auto const& bulk_mesh = process.getMesh();
    auto const& node_property_names =
        bulk_mesh.getProperties().getPropertyVectorNames(
            MeshLib::MeshItemType::Node);
    for (auto const& name : node_property_names)
    {
        addBulkMeshNodePropertyToSubMesh(bulk_mesh, submesh, name);
    }
    return submesh;
}

void Output::doOutputAlways(Process const& process,
                            const int process_id,
                            int const timestep,
                            const double t,
                            int const iteration,
                            std::vector<GlobalVector*> const& xs) const
{
    BaseLib::RunTime time_output;
    time_output.start();

    bool const output_secondary_variables = true;
    auto const process_output_data =
        createProcessOutputData(process, xs.size(), process.getMesh());

    // Need to add variables of process to mesh even if no output takes place.
    addProcessDataToMesh(t, xs, process_id, process_output_data,
                         output_secondary_variables,
                         _output_data_specification);

    if (!isOutputProcess(process_id, process))
    {
        return;
    }

    std::vector<std::reference_wrapper<const MeshLib::Mesh>> output_meshes;
    for (auto const& mesh_output_name : _mesh_names_for_output)
    {
        if (process.getMesh().getName() == mesh_output_name)
        {
            // process related output
            output_meshes.emplace_back(process.getMesh());
        }
        else
        {
            // mesh related output
            auto const& submesh =
                prepareSubmesh(mesh_output_name, process, process_id, t, xs);
            output_meshes.emplace_back(submesh);
        }
    }

    outputMeshes(timestep, t, iteration, std::move(output_meshes));

    INFO("[time] Output of timestep {:d} took {:g} s.", timestep,
         time_output.elapsed());
}

void Output::doOutput(Process const& process,
                      const int process_id,
                      int const timestep,
                      const double t,
                      int const iteration,
                      std::vector<GlobalVector*> const& xs) const
{
    if (_output_data_specification.isOutputStep(timestep, t))
    {
        doOutputAlways(process, process_id, timestep, t, iteration, xs);
    }
#ifdef OGS_USE_INSITU
    // Note: last time step may be output twice: here and in
    // doOutputLastTimestep() which throws a warning.
    InSituLib::CoProcess(process.getMesh(), t, timestep, false,
                         _output_format->directory);
#endif
}

void Output::doOutputLastTimestep(Process const& process,
                                  const int process_id,
                                  int const timestep,
                                  const double t,
                                  int const iteration,
                                  std::vector<GlobalVector*> const& xs) const
{
    if (!_output_data_specification.isOutputStep(timestep, t))
    {
        doOutputAlways(process, process_id, timestep, t, iteration, xs);
    }
#ifdef OGS_USE_INSITU
    InSituLib::CoProcess(process.getMesh(), t, timestep, true,
                         _output_format->directory);
#endif
}

void Output::doOutputNonlinearIteration(
    Process const& process, const int process_id, int const timestep,
    const double t, int const iteration,
    std::vector<GlobalVector*> const& xs) const
{
    if (!_output_nonlinear_iteration_results)
    {
        return;
    }

    BaseLib::RunTime time_output;
    time_output.start();

    bool const output_secondary_variable = true;
    auto const process_output_data =
        createProcessOutputData(process, xs.size(), process.getMesh());

    addProcessDataToMesh(t, xs, process_id, process_output_data,
                         output_secondary_variable, _output_data_specification);

    if (!isOutputProcess(process_id, process))
    {
        return;
    }

    std::string const output_file_name = _output_format->constructFilename(
        process.getMesh().getName(), timestep, t, iteration);

    std::string const output_file_path =
        BaseLib::joinPaths(_output_format->directory, output_file_name);

    DBUG("output iteration results to {:s}", output_file_path);

    if (dynamic_cast<OutputVTKFormat*>(_output_format.get()))
    {
        outputMeshVtk(
            output_file_path, process.getMesh(), _output_format->compression,
            dynamic_cast<OutputVTKFormat*>(_output_format.get())->data_mode);
    }
    else
    {
        DBUG("non-linear iterations can only written in Vtk/VTU format.");
    }
    INFO("[time] Output took {:g} s.", time_output.elapsed());
}

std::vector<std::string> Output::getFileNamesForOutput() const
{
    std::vector<std::string> output_names;
    for (auto const& output_name : _mesh_names_for_output)
    {
        output_names.push_back(
            _output_format->constructFilename(output_name, 0, 0, 0));
    }
    return output_names;
}

std::ostream& operator<<(std::ostream& os, Output const& output)
{
    os << "Output::_output_data_specification:\t"
       << output._output_data_specification;
    os << "Output::_output_format:\t" << *(output._output_format);
    return os;
}

}  // namespace ProcessLib
