/**
 * \file
 * \copyright
 * Copyright (c) 2012-2023, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 */

#pragma once

#include <variant>

namespace MeshLib
{
class Mesh;
}

namespace BaseLib
{
class ConfigTree;
}

namespace NumLib
{
struct NoStabilization;
class IsotropicDiffusionStabilization;
class FullUpwind;
using NumericalStabilization =
    std::variant<NoStabilization, IsotropicDiffusionStabilization, FullUpwind>;
}  // namespace NumLib

namespace NumLib
{
NumericalStabilization createNumericalStabilization(
    MeshLib::Mesh const& mesh, BaseLib::ConfigTree const& config);
}  // namespace NumLib
