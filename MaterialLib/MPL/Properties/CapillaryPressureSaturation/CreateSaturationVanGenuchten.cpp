/**
 * \file
 * \copyright
 * Copyright (c) 2012-2023, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include "BaseLib/ConfigTree.h"
#include "SaturationVanGenuchten.h"

namespace MaterialPropertyLib
{
std::unique_ptr<SaturationVanGenuchten> createSaturationVanGenuchten(
    BaseLib::ConfigTree const& config)
{
    //! \ogs_file_param{properties__property__type}
    config.checkConfigParameter("type", "SaturationVanGenuchten");

    // Second access for storage.
    //! \ogs_file_param{properties__property__name}
    auto property_name = config.peekConfigParameter<std::string>("name");

    DBUG("Create SaturationVanGenuchten medium property {:s}.", property_name);

    auto const residual_liquid_saturation =
        //! \ogs_file_param{properties__property__SaturationVanGenuchten__residual_liquid_saturation}
        config.getConfigParameter<double>("residual_liquid_saturation");
    auto const residual_gas_saturation =
        //! \ogs_file_param{properties__property__SaturationVanGenuchten__residual_gas_saturation}
        config.getConfigParameter<double>("residual_gas_saturation");
    auto const exponent =
        //! \ogs_file_param{properties__property__SaturationVanGenuchten__exponent}
        config.getConfigParameter<double>("exponent");
    //! \ogs_file_param{properties__property__SaturationVanGenuchten__p_b}
    auto const p_b = config.getConfigParameter<double>("p_b");

    return std::make_unique<SaturationVanGenuchten>(
        std::move(property_name), residual_liquid_saturation,
        residual_gas_saturation, exponent, p_b);
}
}  // namespace MaterialPropertyLib
