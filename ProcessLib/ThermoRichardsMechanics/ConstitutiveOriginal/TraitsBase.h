/**
 * \file
 * \copyright
 * Copyright (c) 2012-2022, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#pragma once

#include "MaterialLib/SolidModels/MechanicsBase.h"

namespace ProcessLib::ThermoRichardsMechanics::ConstitutiveOriginal
{
template <int DisplacementDim>
using SolidConstitutiveRelation =
    MaterialLib::Solids::MechanicsBase<DisplacementDim>;
}  // namespace ProcessLib::ThermoRichardsMechanics::ConstitutiveOriginal
