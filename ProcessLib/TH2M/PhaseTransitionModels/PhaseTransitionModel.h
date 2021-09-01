/**
 * \file
 * \copyright
 * Copyright (c) 2012-2021, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 */

#pragma once

#include <map>

#include "MaterialLib/MPL/Medium.h"

namespace ProcessLib
{
namespace TH2M
{
struct PhaseTransitionModelVariables
{
    // gas phase density
    double rhoGR = 0.;
    double rhoCGR = 0.;
    double rhoWGR = 0.;

    // liquid phase density
    double rhoLR = 0.;
    double rhoWLR = 0.;
    double rhoCLR = 0.;

    // water partial pressure in gas phase
    double pWGR = 0;

    // constituent mass and molar fractions
    double xnCG = 0.;
    double xnWG = 0.;
    double xmCG = 0.;
    double xmWG = 0.;
    double xmCL = 0.;
    double xmWL = 0.;

    // molar fraction derivatives
    double dxnCG_dpGR = 0.;
    double dxnCG_dpCap = 0.;
    double dxnCG_dT = 0.;

    // mass fraction derivatives
    double dxmCG_dpGR = 0.;
    double dxmWG_dpGR = 0.;
    double dxmCL_dpLR = 0.;
    double dxmWL_dpLR = 0.;
    double dxmCG_dT = 0.;
    double dxmWG_dT = 0.;
    double dxmCL_dT = 0.;
    double dxmWL_dT = 0.;

    // viscosities
    double muGR = 0.;
    double muLR = 0.;
    // thermal conductivities
    double lambdaGR = 0.;
    double lambdaLR = 0.;

    double diffusion_coefficient_vapour = 0.;
    double diffusion_coefficient_solvate = 0.;

    // specific enthalpies
    double hG = 0;
    double hCG = 0;
    double hWG = 0;
    double hL = 0;

    // specific inner energies
    double uG = 0;
    double uL = 0;
};

struct PhaseTransitionModel
{
    explicit PhaseTransitionModel(
        std::map<int, std::shared_ptr<MaterialPropertyLib::Medium>> const&
            media)
    {
        DBUG("Create phase transition models...");

        // check for minimum requirement definitions in media object
        std::array const required_gas_properties = {
            MaterialPropertyLib::viscosity, MaterialPropertyLib::density,
            MaterialPropertyLib::thermal_conductivity};
        std::array const required_liquid_properties = {
            MaterialPropertyLib::viscosity, MaterialPropertyLib::density,
            MaterialPropertyLib::thermal_conductivity};

        for (auto const& m : media)
        {
            checkRequiredProperties(m.second->phase("Gas"),
                                    required_gas_properties);
            checkRequiredProperties(m.second->phase("AqueousLiquid"),
                                    required_liquid_properties);
        }
    }

    virtual ~PhaseTransitionModel() = default;

    virtual void computeConstitutiveVariables(
        const MaterialPropertyLib::Medium* medium,
        MaterialPropertyLib::VariableArray variables,
        ParameterLib::SpatialPosition pos, double const t, double const dt)
    {
        cv = updateConstitutiveVariables(cv, medium, variables, pos, t, dt);
    }

    virtual PhaseTransitionModelVariables updateConstitutiveVariables(
        PhaseTransitionModelVariables const& phase_transition_model_variables,
        const MaterialPropertyLib::Medium* medium,
        MaterialPropertyLib::VariableArray variables,
        ParameterLib::SpatialPosition pos, double const t,
        double const dt) const = 0;

    int numberOfComponents(
        std::map<int, std::shared_ptr<MaterialPropertyLib::Medium>> const&
            media,
        std::string phase_name)
    {
        // Always the first (begin) medium that holds fluid phases.
        auto const medium = media.begin()->second;
        return medium->phase(phase_name).numberOfComponents();
    }

    int findComponentIndex(
        std::map<int, std::shared_ptr<MaterialPropertyLib::Medium>> const&
            media,
        std::string phase_name, MaterialPropertyLib::PropertyType property_type)
    {
        // It is always the first (begin) medium that holds fluid phases.
        auto const medium = media.begin()->second;
        auto const& phase = medium->phase(phase_name);

        // find the component for which the property 'property_type' is defined
        for (std::size_t c = 0; c < phase.numberOfComponents(); c++)
        {
            if (phase.component(c).hasProperty(property_type))
            {
                return c;
            }
        }

        // A lot of checks can (and should) be done to make sure that the right
        // components with the right properties are used. For example, the names
        // of the components can be compared to check that the name of the
        // evaporable component does not also correspond to the name of the
        // solvate.

        OGS_FATAL(
            "PhaseTransitionModel::findComponentIndex could not find the "
            "specified property type in the phase.");
    }

    // constitutive variables
    PhaseTransitionModelVariables cv;
};

}  // namespace TH2M
}  // namespace ProcessLib
