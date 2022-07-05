/**
 * \file
 * \copyright
 * Copyright (c) 2012-2022, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include "SolidMechanics.h"

namespace ProcessLib::ThermoRichardsMechanics
{
template <int DisplacementDim>
void SolidMechanicsModel<DisplacementDim>::eval(
    const SpaceTimeData& x_t,
    SolidThermalExpansionData<DisplacementDim> const& s_therm_exp_data,
    SwellingDataStateless<DisplacementDim> const& swelling_data,
    TemperatureData<DisplacementDim> const& T_data,
    CapillaryPressureData<DisplacementDim> const& p_cap_data,
    BiotData const& biot_data,
    BishopsData const& bishops_data,
    StrainData<DisplacementDim> const& eps_data,
    StrainData<DisplacementDim> const& eps_prev_data,
    MaterialStateData<DisplacementDim>& mat_state,
    SolidMechanicsDataStateful<DisplacementDim> const& prev_state,
    SolidMechanicsDataStateful<DisplacementDim>& current_state,
    SolidMechanicsDataStateless<DisplacementDim>& out) const
{
    namespace MPL = MaterialPropertyLib;
    MPL::VariableArray variables;

    double const dT = T_data.T_dot * x_t.dt;
    double const T_prev = T_data.T - dT;

    current_state.eps_m.noalias() =
        prev_state.eps_m + eps_data.eps - eps_prev_data.eps -
        s_therm_exp_data.solid_linear_thermal_expansivity_vector * dT +
        swelling_data.eps_m;

    variables[static_cast<int>(MPL::Variable::mechanical_strain)]
        .emplace<KelvinVector<DisplacementDim>>(current_state.eps_m);

    MPL::VariableArray variables_prev;
    variables_prev[static_cast<int>(MPL::Variable::stress)]
        .emplace<KelvinVector<DisplacementDim>>(prev_state.sigma_eff);
    variables_prev[static_cast<int>(MPL::Variable::mechanical_strain)]
        .emplace<KelvinVector<DisplacementDim>>(prev_state.eps_m);
    variables_prev[static_cast<int>(MPL::Variable::temperature)]
        .emplace<double>(T_prev);

    auto solution = solid_material_.integrateStress(
        variables_prev, variables, x_t.t, x_t.x, x_t.dt,
        *mat_state.material_state_variables);

    if (!solution)
    {
        OGS_FATAL("Computation of local constitutive relation failed.");
    }

    std::tie(current_state.sigma_eff, mat_state.material_state_variables,
             out.stiffness_tensor) = std::move(*solution);

    auto const& identity2 = MathLib::KelvinVector::Invariants<
        MathLib::KelvinVector::kelvin_vector_dimensions(
            DisplacementDim)>::identity2;
    out.sigma_total =
        current_state.sigma_eff +
        biot_data.alpha * bishops_data.chi_S_L * p_cap_data.p_cap * identity2;

    out.J_uT_BT_K_N =  // TODO is this thermal stress?
        -out.stiffness_tensor *
        s_therm_exp_data.solid_linear_thermal_expansivity_vector;

    out.equivalent_plastic_strain =
        mat_state.material_state_variables->getEquivalentPlasticStrain();
}

template struct SolidMechanicsModel<2>;
template struct SolidMechanicsModel<3>;

}  // namespace ProcessLib::ThermoRichardsMechanics
