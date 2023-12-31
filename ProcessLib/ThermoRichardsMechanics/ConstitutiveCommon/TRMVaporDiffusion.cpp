/**
 * \file
 * \copyright
 * Copyright (c) 2012-2023, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include "TRMVaporDiffusion.h"

namespace ProcessLib::ThermoRichardsMechanics
{
template <int DisplacementDim>
void TRMVaporDiffusionData<DisplacementDim>::setZero()
{
    volumetric_heat_capacity_vapor = 0;
    vapor_velocity = GlobalDimVector<DisplacementDim>::Zero(DisplacementDim);
    storage_coefficient_by_water_vapor = 0;

    J_pT_X_dNTdN = 0;
    K_pp_X_dNTdN = 0;
    K_TT_X_dNTdN = 0;
    K_Tp_X_dNTdN = 0;
    M_Tp_X_NTN = 0;
    M_TT_X_NTN = 0;
    M_pT_X_NTN = 0;
}

template <int DisplacementDim>
void TRMVaporDiffusionModel<DisplacementDim>::eval(
    SpaceTimeData const& x_t, MediaData const& media_data,
    LiquidDensityData const& rho_L_data, SaturationData const& S_L_data,
    SaturationDataDeriv const& dS_L_data, PorosityData const& poro_data,
    CapillaryPressureData<DisplacementDim> const& p_cap_data,
    TemperatureData<DisplacementDim> const& T_data,
    TRMVaporDiffusionData<DisplacementDim>& out) const
{
    namespace MPL = MaterialPropertyLib;
    MPL::VariableArray variables;
    variables.temperature = T_data.T;
    variables.phase_pressure = -p_cap_data.p_cap;
    variables.density = rho_L_data.rho_LR;
    variables.liquid_saturation = S_L_data.S_L;

    auto const& liquid_phase = media_data.liquid;

    out.setZero();

    if (liquid_phase.hasProperty(MPL::PropertyType::vapour_diffusion) &&
        S_L_data.S_L < 1.0)
    {
        double const rho_wv =
            liquid_phase.property(MaterialPropertyLib::vapour_density)
                .template value<double>(variables, x_t.x, x_t.t, x_t.dt);

        double const drho_wv_dT =
            liquid_phase.property(MaterialPropertyLib::vapour_density)
                .template dValue<double>(variables, MPL::Variable::temperature,
                                         x_t.x, x_t.t, x_t.dt);
        double const drho_wv_dp =
            liquid_phase.property(MaterialPropertyLib::vapour_density)
                .template dValue<double>(variables,
                                         MPL::Variable::phase_pressure, x_t.x,
                                         x_t.t, x_t.dt);
        auto const f_Tv =
            liquid_phase
                .property(
                    MPL::PropertyType::thermal_diffusion_enhancement_factor)
                .template value<double>(variables, x_t.x, x_t.t, x_t.dt);

        double const phi = poro_data.phi;
        variables.porosity = phi;
        double const D_v =
            liquid_phase.property(MPL::PropertyType::vapour_diffusion)
                .template value<double>(variables, x_t.x, x_t.t, x_t.dt);

        out.J_pT_X_dNTdN = f_Tv * D_v * drho_wv_dT;
        out.K_pp_X_dNTdN = D_v * drho_wv_dp;

        if (auto const& medium = media_data.medium; medium.hasPhase("Gas"))
        {
            if (auto const& gas_phase = medium.phase("Gas");
                gas_phase.hasProperty(
                    MPL::PropertyType::specific_heat_capacity))
            {
                // Vapour velocity
                out.vapor_velocity =
                    -(out.J_pT_X_dNTdN * T_data.grad_T -
                      out.K_pp_X_dNTdN * p_cap_data.grad_p_cap) /
                    rho_L_data.rho_LR;
                double const specific_heat_capacity_vapor =
                    gas_phase
                        .property(MaterialPropertyLib::PropertyType::
                                      specific_heat_capacity)
                        .template value<double>(variables, x_t.x, x_t.t,
                                                x_t.dt);

                out.volumetric_heat_capacity_vapor =
                    rho_wv * specific_heat_capacity_vapor;
                out.M_TT_X_NTN += out.volumetric_heat_capacity_vapor *
                                  (1 - S_L_data.S_L) * phi;
            }
        }

        out.storage_coefficient_by_water_vapor =
            phi *
            (rho_wv * dS_L_data.dS_L_dp_cap + (1 - S_L_data.S_L) * drho_wv_dp);

        out.M_pT_X_NTN += phi * (1 - S_L_data.S_L) * drho_wv_dT;

        //
        // Latent heat term
        //
        if (liquid_phase.hasProperty(MPL::PropertyType::latent_heat))
        {
            double const factor = phi * (1 - S_L_data.S_L) / rho_L_data.rho_LR;
            // The volumetric latent heat of vaporization of liquid water
            double const L0 =
                liquid_phase.property(MPL::PropertyType::latent_heat)
                    .template value<double>(variables, x_t.x, x_t.t, x_t.dt) *
                rho_L_data.rho_LR;

            double const rho_wv_over_rho_L = rho_wv / rho_L_data.rho_LR;

            out.M_TT_X_NTN +=
                factor * L0 *
                (drho_wv_dT - rho_wv_over_rho_L * rho_L_data.drho_LR_dT);
            out.M_Tp_X_NTN =
                (factor * L0 *
                     (drho_wv_dp - rho_wv_over_rho_L * rho_L_data.drho_LR_dp) +
                 L0 * phi * rho_wv_over_rho_L * dS_L_data.dS_L_dp_cap);
            out.K_TT_X_dNTdN = L0 * out.J_pT_X_dNTdN / rho_L_data.rho_LR;
            out.K_Tp_X_dNTdN = L0 * out.K_pp_X_dNTdN / rho_L_data.rho_LR;
        }
    }
}

template struct TRMVaporDiffusionData<2>;
template struct TRMVaporDiffusionData<3>;
template struct TRMVaporDiffusionModel<2>;
template struct TRMVaporDiffusionModel<3>;
}  // namespace ProcessLib::ThermoRichardsMechanics
