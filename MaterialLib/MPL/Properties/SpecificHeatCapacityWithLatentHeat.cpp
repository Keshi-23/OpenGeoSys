/**
 * \file
 * \copyright
 * Copyright (c) 2012-2022, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 * Created on May 20, 2022
 */

#include "SpecificHeatCapacityWithLatentHeat.h"

#include "MaterialLib/MPL/Medium.h"

namespace MaterialPropertyLib
{
SpecificHeatCapacityWithLatentHeat::SpecificHeatCapacityWithLatentHeat(
    std::string name, double const l)
    : l_(l)
{
    name_ = std::move(name);
}

void SpecificHeatCapacityWithLatentHeat::checkScale() const
{
    if (!std::holds_alternative<Medium*>(scale_))
    {
        OGS_FATAL(
            "The property 'SpecificHeatCapacityWithLatentHeat' is "
            "implemented on the 'medium' scale only.");
    }
}

void SpecificHeatCapacityWithLatentHeat::setProperties(
    std::vector<std::unique_ptr<Phase>> const& phases)
{
    // run over phases, identify them and get properties
    for (auto& phase : phases)
    {
        if (phase == nullptr)
        {
            OGS_FATAL(
                "One of the required phases (AqueousLiquid/FrozenLiquid/Solid) "
                "does not exist!");
        }
        std::string phase_name = phase->name;

        auto const& density_property =
            phase->property(MaterialPropertyLib::PropertyType::density);
        auto const& specific_heat_capacity_property = phase->property(
            MaterialPropertyLib::PropertyType::specific_heat_capacity);

        if (phase_name == "AqueousLiquid")
        {
            densities_.liquid = &density_property;
            spec_heat_capacities_.liquid = &specific_heat_capacity_property;
        }
        else if (phase_name == "FrozenLiquid")
        {
            densities_.frozen = &density_property;
            spec_heat_capacities_.frozen = &specific_heat_capacity_property;
        }
        else if (phase_name == "Solid")
        {
            densities_.porous = &density_property;
            spec_heat_capacities_.porous = &specific_heat_capacity_property;
        }
    }
}

double SpecificHeatCapacityWithLatentHeat::effectiveVolumetricHeatCapacity(
    VariableArray const& variable_array,
    ParameterLib::SpatialPosition const& pos, double const t,
    double const dt) const
{
    auto const& medium = *std::get<Medium*>(scale_);
    auto const& porosity_property = medium[PropertyType::porosity];
    auto const& frozen_fraction_property =
        medium[PropertyType::volume_fraction];

    auto const phi =
        std::get<double>(porosity_property.value(variable_array, pos, t, dt));
    auto const pfr = std::get<double>(
        frozen_fraction_property.value(variable_array, pos, t, dt));
    auto const pli = phi - pfr;
    auto const ppo = 1 - phi;

    auto const rho_li =
        std::get<double>(densities_.liquid->value(variable_array, pos, t, dt));
    auto const rho_fr =
        std::get<double>(densities_.frozen->value(variable_array, pos, t, dt));
    auto const rho_po =
        std::get<double>(densities_.porous->value(variable_array, pos, t, dt));

    auto const c_li = std::get<double>(
        spec_heat_capacities_.liquid->value(variable_array, pos, t, dt));
    auto const c_fr = std::get<double>(
        spec_heat_capacities_.frozen->value(variable_array, pos, t, dt));
    auto const c_po = std::get<double>(
        spec_heat_capacities_.porous->value(variable_array, pos, t, dt));

    // rule of mixtures for resulting volumetric heat capacity
    // (mass fraction average of specific heat capacities!)
    return pli * rho_li * c_li + pfr * rho_fr * c_fr + ppo * rho_po * c_po;
}

PropertyDataType SpecificHeatCapacityWithLatentHeat::value(
    VariableArray const& variable_array,
    ParameterLib::SpatialPosition const& pos, double const t,
    double const dt) const
{
    auto const& medium = *std::get<Medium*>(scale_);
    auto const& effective_density_property = medium[PropertyType::density];
    auto const& frozen_fraction_property =
        medium[PropertyType::volume_fraction];

    auto const rho_ef = effective_density_property.template value<double>(
        variable_array, pos, t, dt);
    auto const dpfr_dT = frozen_fraction_property.template dValue<double>(
        variable_array, Variable::temperature, pos, t, dt);

    auto const Cvol =
        effectiveVolumetricHeatCapacity(variable_array, pos, t, dt);
    auto const Lvol = l_ * rho_ef;
    auto const Cvol_app = Cvol - Lvol * dpfr_dT;
    // divide volumetric quantity by density in order to obtain specific value
    return Cvol_app / rho_ef;
}

PropertyDataType SpecificHeatCapacityWithLatentHeat::dValue(
    VariableArray const& variable_array, Variable const variable,
    ParameterLib::SpatialPosition const& pos, double const t,
    double const dt) const
{
    (void)variable;
    assert((variable == Variable::temperature) &&
           "SpecificHeatCapacityWithLatentHeat::dvalue is implemented for "
           "derivatives with respect to temperature only.");

    auto const& medium = *std::get<Medium*>(scale_);
    auto const& effective_density_property = medium[PropertyType::density];
    auto const& frozen_fraction_property =
        medium[PropertyType::volume_fraction];

    auto const rho_ef = effective_density_property.template value<double>(
        variable_array, pos, t, dt);
    auto const rho_li =
        std::get<double>(densities_.liquid->value(variable_array, pos, t, dt));
    auto const rho_fr =
        std::get<double>(densities_.frozen->value(variable_array, pos, t, dt));
    auto const c_li = std::get<double>(
        spec_heat_capacities_.liquid->value(variable_array, pos, t, dt));
    auto const c_fr = std::get<double>(
        spec_heat_capacities_.frozen->value(variable_array, pos, t, dt));
    auto const drho_dT = effective_density_property.template dValue<double>(
        variable_array, Variable::temperature, pos, t, dt);
    auto const dpfr_dT = frozen_fraction_property.template dValue<double>(
        variable_array, Variable::temperature, pos, t, dt);
    auto const d2pfr_dT2 = frozen_fraction_property.template d2Value<double>(
        variable_array, Variable::temperature, Variable::temperature, pos, t,
        dt);
    auto const Cvol =
        effectiveVolumetricHeatCapacity(variable_array, pos, t, dt);
    // TODO: avoid duplicate code, call value()?
    auto const C_app = (Cvol - l_ * rho_ef * dpfr_dT) / rho_ef;
    auto const dCvol_dpfr = rho_fr * c_fr - rho_li * c_li;
    auto const dCvol_app_dT = dCvol_dpfr * dpfr_dT - l_ * rho_ef * d2pfr_dT2;

    return (dCvol_app_dT - drho_dT / rho_ef * C_app) / rho_ef;
}
}  // namespace MaterialPropertyLib
