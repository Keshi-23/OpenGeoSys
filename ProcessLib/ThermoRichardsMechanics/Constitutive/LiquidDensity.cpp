/**
 * \file
 * \copyright
 * Copyright (c) 2012-2022, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include "LiquidDensity.h"

namespace ProcessLib::ThermoRichardsMechanics
{
template <int DisplacementDim>
void LiquidDensityModel<DisplacementDim>::eval(
    SpaceTimeData const& x_t, MediaData const& media_data,
    CapillaryPressureData<DisplacementDim> const& p_cap_data,
    TemperatureData<DisplacementDim> const& T_data,
    LiquidDensityData& out) const
{
    namespace MPL = MaterialPropertyLib;
    MPL::VariableArray variables;
    variables[static_cast<int>(MPL::Variable::phase_pressure)] =
        -p_cap_data.p_cap;
    variables[static_cast<int>(MPL::Variable::temperature)] = T_data.T;

    auto const& liquid_phase = media_data.liquid;

    out.rho_LR = liquid_phase.property(MPL::PropertyType::density)
                     .template value<double>(variables, x_t.x, x_t.t, x_t.dt);

    out.drho_LR_dp =
        liquid_phase.property(MPL::PropertyType::density)
            .template dValue<double>(variables, MPL::Variable::phase_pressure,
                                     x_t.x, x_t.t, x_t.dt);

    out.drho_LR_dT =
        liquid_phase.property(MPL::PropertyType::density)
            .template dValue<double>(variables, MPL::Variable::temperature,
                                     x_t.x, x_t.t, x_t.dt);
}

template struct LiquidDensityModel<2>;
template struct LiquidDensityModel<3>;
}  // namespace ProcessLib::ThermoRichardsMechanics
