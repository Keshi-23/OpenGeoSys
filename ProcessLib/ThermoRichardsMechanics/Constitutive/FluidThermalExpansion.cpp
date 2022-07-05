/**
 * \file
 * \copyright
 * Copyright (c) 2012-2022, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include "FluidThermalExpansion.h"

#include "MaterialLib/MPL/Utils/FormKelvinVectorFromThermalExpansivity.h"
#include "MaterialLib/MPL/Utils/GetLiquidThermalExpansivity.h"

namespace ProcessLib::ThermoRichardsMechanics
{
template <int DisplacementDim>
void FluidThermalExpansionModel<DisplacementDim>::eval(
    SpaceTimeData const& x_t, MediaData const& media_data,
    SolidThermalExpansionData<DisplacementDim> const& s_therm_exp_data,
    PorosityData const& poro_data, LiquidDensityData const& rho_L_data,
    BiotData const& biot_data, FluidThermalExpansionData& out) const
{
    static constexpr int kelvin_vector_size =
        MathLib::KelvinVector::kelvin_vector_dimensions(DisplacementDim);
    using Invariants = MathLib::KelvinVector::Invariants<kelvin_vector_size>;

    namespace MPL = MaterialPropertyLib;
    MPL::VariableArray variables;

    double const phi = poro_data.phi;
    double const alpha = biot_data.alpha;

    double const fluid_volumetric_thermal_expansion =
        phi * MPL::getLiquidThermalExpansivity(media_data.liquid, variables,
                                               rho_L_data.rho_LR, x_t.x, x_t.t,
                                               x_t.dt);

    out.eff_thermal_expansion =
        (alpha - phi) *
            Invariants::trace(
                s_therm_exp_data.solid_linear_thermal_expansivity_vector) +
        fluid_volumetric_thermal_expansion;
}

template struct FluidThermalExpansionModel<2>;
template struct FluidThermalExpansionModel<3>;
}  // namespace ProcessLib::ThermoRichardsMechanics
