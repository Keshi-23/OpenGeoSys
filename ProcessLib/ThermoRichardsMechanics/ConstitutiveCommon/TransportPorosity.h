/**
 * \file
 * \copyright
 * Copyright (c) 2012-2023, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#pragma once

#include "ProcessLib/ThermoRichardsMechanics/ConstitutiveCommon/Bishops.h"
#include "ProcessLib/ThermoRichardsMechanics/ConstitutiveCommon/Porosity.h"
#include "ProcessLib/ThermoRichardsMechanics/ConstitutiveCommon/SolidCompressibility.h"

namespace ProcessLib::ThermoRichardsMechanics
{

struct TransportPorosityData
{
    double phi;

    static auto reflect()
    {
        return ProcessLib::Reflection::reflectWithName(
            "transport_porosity", &TransportPorosityData::phi);
    }
};

template <int DisplacementDim>
struct TransportPorosityModel
{
    void eval(SpaceTimeData const& x_t, MediaData const& media_data,
              SolidCompressibilityData const& solid_compressibility_data,
              BishopsData const& bishops_data,
              BishopsData const& bishops_data_prev,
              CapillaryPressureData<DisplacementDim> const& p_cap_data,
              PorosityData const& poro_data,
              StrainData<DisplacementDim> const& eps_data,
              StrainData<DisplacementDim> const& eps_prev_data,
              TransportPorosityData const& transport_poro_data_prev,
              TransportPorosityData& transport_poro_data) const;
};

extern template struct TransportPorosityModel<2>;
extern template struct TransportPorosityModel<3>;
}  // namespace ProcessLib::ThermoRichardsMechanics
