/**
 * \brief  Declaration of class Simulation
 * \file
 *
 * \copyright
 * Copyright (c) 2012-2022, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include "Applications/ApplicationsLib/LinearSolverLibrarySetup.h"
#include "Applications/ApplicationsLib/TestDefinition.h"

class ProjectData;

class Simulation final
{
public:
    Simulation(int argc, char* argv[]);

    void initializeDataStructures(
        std::string const& project,
        std::vector<std::string> const& xml_patch_file_names,
        bool reference_path_is_set, std::string const& reference_path,
        bool nonfatal, std::string const& outdir, std::string const& mesh_dir,
        bool write_prj);

    double currentTime() const;
    double endTime() const;
    bool executeTimeStep();
    bool executeSimulation();

    std::optional<ApplicationsLib::TestDefinition> getTestDefinition();

    ~Simulation();

private:
    ApplicationsLib::LinearSolverLibrarySetup linear_solver_library_setup;
    std::unique_ptr<ProjectData> project_data;
    std::optional<ApplicationsLib::TestDefinition> test_definition;
};
