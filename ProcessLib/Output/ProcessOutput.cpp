/**
 * \file
 * \copyright
 * Copyright (c) 2012-2021, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include "ProcessOutput.h"

#ifndef _WIN32
#ifndef __APPLE__
#include <cfenv>
#endif  // __APPLE__
#endif  // _WIN32

#include "InfoLib/GitInfo.h"
#include "IntegrationPointWriter.h"
#ifdef USE_PETSC
#include "MeshLib/NodePartitionedMesh.h"
#endif
#include "MeshLib/IO/VtkIO/VtuInterface.h"
#include "NumLib/DOF/LocalToGlobalIndexMap.h"
#include "ProcessLib/Process.h"

/// Copies the ogs_version string containing the release number and the git
/// hash.
static void addOgsVersion(MeshLib::Mesh& mesh)
{
    auto& ogs_version_field = *MeshLib::getOrCreateMeshProperty<char>(
        mesh, GitInfoLib::GitInfo::OGS_VERSION,
        MeshLib::MeshItemType::IntegrationPoint, 1);

    ogs_version_field.assign(GitInfoLib::GitInfo::ogs_version.begin(),
                             GitInfoLib::GitInfo::ogs_version.end());
}

static void addSecondaryVariableNodes(
    double const t,
    std::vector<GlobalVector*> const& x,
    std::vector<NumLib::LocalToGlobalIndexMap const*> const& dof_table,
    ProcessLib::SecondaryVariable const& var,
    std::string const& output_name,
    MeshLib::Mesh& mesh)
{
    DBUG("  secondary variable {:s}", output_name);

    auto& nodal_values_mesh = *MeshLib::getOrCreateMeshProperty<double>(
        mesh, output_name, MeshLib::MeshItemType::Node,
        var.fcts.num_components);
    if (nodal_values_mesh.size() !=
        mesh.getNumberOfNodes() * var.fcts.num_components)
    {
        OGS_FATAL(
            "Nodal property `{:s}' does not have the right number of "
            "components. Expected: {:d}, actual: {:d}",
            output_name,
            mesh.getNumberOfNodes() * var.fcts.num_components,
            nodal_values_mesh.size());
    }

    std::unique_ptr<GlobalVector> result_cache;
    auto const& nodal_values =
        var.fcts.eval_field(t, x, dof_table, result_cache);

    // Copy result
    nodal_values.copyValues(nodal_values_mesh);
}

static void addSecondaryVariableResiduals(
    double const t,
    std::vector<GlobalVector*> const& x,
    std::vector<NumLib::LocalToGlobalIndexMap const*> const& dof_table,
    ProcessLib::SecondaryVariable const& var,
    std::string const& output_name,
    MeshLib::Mesh& mesh)
{
    if (!var.fcts.eval_residuals)
    {
        return;
    }

    DBUG("  secondary variable {:s} residual", output_name);
    auto const& property_name_res = output_name + "_residual";

    auto& residuals_mesh = *MeshLib::getOrCreateMeshProperty<double>(
        mesh, property_name_res, MeshLib::MeshItemType::Cell,
        var.fcts.num_components);
    if (residuals_mesh.size() !=
        mesh.getNumberOfElements() * var.fcts.num_components)
    {
        OGS_FATAL(
            "Cell property `{:s}' does not have the right number of "
            "components. Expected: {:d}, actual: {:d}",
            property_name_res,
            mesh.getNumberOfElements() * var.fcts.num_components,
            residuals_mesh.size());
    }

    std::unique_ptr<GlobalVector> result_cache;
    auto const& residuals =
        var.fcts.eval_residuals(t, x, dof_table, result_cache);
#ifdef USE_PETSC
    std::size_t const global_vector_size =
        residuals.getLocalSize() + residuals.getGhostSize();
#else
    std::size_t const global_vector_size = residuals.size();
#endif
    if (residuals_mesh.size() != global_vector_size)
    {
        OGS_FATAL(
            "The residual of secondary variable `{:s}' did not evaluate to the "
            "right number of components. Expected: {:d}, actual: {:d}.",
            var.name, residuals_mesh.size(), global_vector_size);
    }

    // Copy result
    residuals.copyValues(residuals_mesh);
}

std::vector<double> copySolutionVector(GlobalVector const& x)
{
#ifdef USE_PETSC
    // TODO It is also possible directly to copy the data for single process
    // variable to a mesh property. It needs a vector of global indices and
    // some PETSc magic to do so.
    std::vector<double> x_copy(x.getLocalSize() + x.getGhostSize());
#else
    std::vector<double> x_copy(x.size());
#endif

    x.copyValues(x_copy);

    return x_copy;
}

std::set<std::string> addPrimaryVariablesToMesh(
    MeshLib::Mesh& mesh,
    GlobalVector const& x,
    std::vector<std::reference_wrapper<ProcessLib::ProcessVariable>> const&
        process_variables,
    std::set<std::string> const& output_variables,
    NumLib::LocalToGlobalIndexMap const& dof_table,
    [[maybe_unused]] NumLib::LocalToGlobalIndexMap const& bulk_dof_table)
{
    auto const x_copy = copySolutionVector(x);
    std::set<std::string> already_output;

    const auto number_of_dof_variables = dof_table.getNumberOfVariables();

    int global_component_offset = 0;
    int global_component_offset_next = 0;

    auto output_single_component_at_node =
        [&x, &x_copy, &global_component_offset, &dof_table](
            MeshLib::PropertyVector<double>& output_data, std::size_t mesh_id,
            MeshLib::Node const& node, int component_id, int n_components)
    {
        auto const node_id = node.getID();
        MeshLib::Location const loc(mesh_id, MeshLib::MeshItemType::Node,
                                    node_id);

        auto const global_component_id = global_component_offset + component_id;
        auto const index = dof_table.getLocalIndex(
            loc, global_component_id, x.getRangeBegin(), x.getRangeEnd());

        output_data[node_id * n_components + component_id] = x_copy[index];
    };

#ifdef USE_PETSC
    MeshLib::PropertyVector<std::size_t>* bulk_node_id_map = nullptr;

    if (&bulk_dof_table != &dof_table)
    {
        if (!mesh.getProperties().existsPropertyVector<std::size_t>(
                "bulk_node_ids"))
        {
            OGS_FATAL(
                "The required bulk node ids map does not exist in "
                "the boundary mesh '{:s}' or has the wrong data "
                "type (should be equivalent to C++ data type "
                "std::size_t which is an unsigned integer of size "
                "{:d} or UInt64 in vtk terminology).",
                mesh.getName(), sizeof(std::size_t));
        }
        bulk_node_id_map = mesh.getProperties().getPropertyVector<std::size_t>(
            "bulk_node_ids");
    }

    auto output_single_component_at_ghost_node =
        [&x, &x_copy, &global_component_offset, &dof_table, &bulk_dof_table,
         bulk_node_id_map, &mesh](MeshLib::PropertyVector<double>& output_data,
                                  MeshLib::Node const& node, int component_id,
                                  int n_components)
    {
        if (&bulk_dof_table != &dof_table)
        {
            auto const node_id = node.getID();

            if (static_cast<MeshLib::NodePartitionedMesh const&>(mesh)
                    .isGhostNode(node_id))
            {
                auto const bulk_node_id = (*bulk_node_id_map)[node_id];
                // use bulk_dof_tables to find information
                std::size_t const bulk_mesh_id = 0;
                MeshLib::Location const loc(
                    bulk_mesh_id, MeshLib::MeshItemType::Node, bulk_node_id);
                auto const global_component_id =
                    global_component_offset + component_id;
                auto const index = bulk_dof_table.getLocalIndex(
                    loc, global_component_id, x.getRangeBegin(),
                    x.getRangeEnd());

                output_data[node_id * n_components + component_id] =
                    x_copy[index];

                return true;
            }
        }

        return false;
    };

#endif

    for (int variable_id = 0;
         variable_id < static_cast<int>(process_variables.size());
         ++variable_id)
    {
        ProcessLib::ProcessVariable& pv = process_variables[variable_id];
        int const n_components = pv.getNumberOfGlobalComponents();
        // If (number_of_dof_variables==1), the case is either the staggered
        // scheme being applied or a single PDE being solved.
        const int sub_meshset_id =
            (number_of_dof_variables == 1) ? 0 : variable_id;

        if (number_of_dof_variables > 1)
        {
            global_component_offset = global_component_offset_next;
            global_component_offset_next += n_components;
        }

        if (output_variables.find(pv.getName()) == output_variables.cend())
        {
            continue;
        }

        already_output.insert(pv.getName());

        DBUG("  process variable {:s}", pv.getName());

        auto const num_comp = pv.getNumberOfGlobalComponents();
        auto& output_data = *MeshLib::getOrCreateMeshProperty<double>(
            mesh, pv.getName(), MeshLib::MeshItemType::Node, num_comp);

        for (int component_id = 0; component_id < num_comp; ++component_id)
        {
            auto const& mesh_subset =
                dof_table.getMeshSubset(sub_meshset_id, component_id);
            auto const mesh_id = mesh_subset.getMeshID();
            for (auto const* node : mesh_subset.getNodes())
            {
#ifdef USE_PETSC

                if (output_single_component_at_ghost_node(
                        output_data, *node, component_id, n_components))
                {
                    continue;
                }
#endif

                output_single_component_at_node(output_data, mesh_id, *node,
                                                component_id, n_components);
            }
        }
    }

    return already_output;
}

namespace ProcessLib
{
void addProcessDataToMesh(
    const double t, std::vector<GlobalVector*> const& xs, int const process_id,
    MeshLib::Mesh& mesh,
    std::vector<NumLib::LocalToGlobalIndexMap const*> const& bulk_dof_tables,
    std::vector<NumLib::LocalToGlobalIndexMap const*> const& dof_tables,
    Process const& process, bool const output_secondary_variable,
    OutputDataSpecification const& process_output)
{
    DBUG("Process output data.");

    auto const& process_variables = process.getProcessVariables(process_id);
    auto const& secondary_variables = process.getSecondaryVariables();
    auto const* const integration_point_writers =
        process.getIntegrationPointWriter(mesh);

    addOgsVersion(mesh);

    auto const& output_variables = process_output.output_variables;
    std::set<std::string> already_output = addPrimaryVariablesToMesh(
        mesh, *xs[process_id], process_variables, output_variables,
        *dof_tables[process_id], *bulk_dof_tables[process_id]);

    if (output_secondary_variable)
    {
        for (auto const& external_variable_name : secondary_variables)
        {
            auto const& name = external_variable_name.first;
            if (!already_output.insert(name).second)
            {
                // no insertion took place, output already done
                continue;
            }

            addSecondaryVariableNodes(
                t, xs, dof_tables, secondary_variables.get(name), name, mesh);

            if (process_output.output_residuals)
            {
                addSecondaryVariableResiduals(t, xs, dof_tables,
                                              secondary_variables.get(name),
                                              name, mesh);
            }
        }
    }

    if (integration_point_writers)
    {
        addIntegrationPointDataToMesh(mesh, *integration_point_writers);
    }
}

void makeOutput(std::string const& file_name, MeshLib::Mesh const& mesh,
                bool const compress_output, int const data_mode)
{
    // Write output file
    DBUG("Writing output to '{:s}'.", file_name);

    // Store floating-point exception handling. Output of NaN's triggers
    // floating point exceptions. Because we are not debugging VTK (or other
    // libraries) at this point, the possibly set exceptions are temporary
    // disabled and restored before end of the function.
#ifndef _WIN32
#ifndef __APPLE__
    fenv_t fe_env;
    fegetenv(&fe_env);
    fesetenv(FE_DFL_ENV);  // Set default environment effectively disabling
                           // exceptions.
#endif                     //_WIN32
#endif                     //__APPLE__

    MeshLib::IO::VtuInterface vtu_interface(&mesh, data_mode, compress_output);
    vtu_interface.writeToFile(file_name);

    // Restore floating-point exception handling.
#ifndef _WIN32
#ifndef __APPLE__
    fesetenv(&fe_env);
#endif  //_WIN32
#endif  //__APPLE__
}
}  // namespace ProcessLib
