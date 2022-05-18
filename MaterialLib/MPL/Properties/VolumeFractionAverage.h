/**
 * \file
 * \copyright
 * Copyright (c) 2012-2021, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 */
#pragma once

#include <array>

#include "MaterialLib/MPL/Phase.h"
#include "MaterialLib/MPL/Property.h"

namespace MaterialPropertyLib
{
/**
 * Effective medium property obtained by volume fraction averaging
 *
 * \details This property must be a medium property. It computes the effective
 * property based on the phase properties as a volume fraction weighted average
 * in the following form
 *
 * \f$
 *      X_{\mathrm{eff}} = (1-\phi) X_\mathrm{pR} +
 *\phi_\mathrm{f}\,X_\mathrm{fR}
 *                       + (\phi - \phi_\mathrm{f}) X_\mathrm{lR}
 * \f$
 *
 * where $\phi$ is the porosity (pore space volume fraction), $\phi_\text{f}$
 * is the frozen volume fraction and $\phi_\text{l}=\phi - \phi_\mathrm{f}$ is
 * the liquid one. $\mathrm{R}$ stands for real/pure phase property.
 *
 **/
class VolumeFractionAverage : public Property
{
    // Struct with pointers on the properties of relevant phases
    struct PhaseProperties
    {
        const Property* liquid;
        const Property* frozen;
        const Property* porous;
    };

public:
    explicit VolumeFractionAverage(std::string name);

    void checkScale() const override;

    // initialize container with pointers to the phases properties
    void setProperties(
        std::vector<std::unique_ptr<Phase>> const& phases) override;

    PropertyDataType value(VariableArray const& variable_array,
                           ParameterLib::SpatialPosition const& pos,
                           double const t,
                           double const dt) const override;

    PropertyDataType dValue(VariableArray const& variable_array,
                            Variable const variable,
                            ParameterLib::SpatialPosition const& pos,
                            double const t,
                            double const dt) const override;

protected:  // order matters!
    PropertyType prop_type_;
    PhaseProperties properties_;
};
}  // namespace MaterialPropertyLib
