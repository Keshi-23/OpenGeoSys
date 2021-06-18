/**
 * \file
 * \copyright
 * Copyright (c) 2012-2021, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#pragma once

#include <Eigen/Eigen>
#include <memory>

#include "MeshLib/PropertyVector.h"

namespace MaterialPropertyLib
{
class MaterialSpatialDistributionMap;
}

namespace ProcessLib
{
namespace StokesFlow
{
struct StokesFlowProcessData
{
    std::unique_ptr<MaterialPropertyLib::MaterialSpatialDistributionMap>
        media_map;
    /// an external force that applies in the bulk of the fluid, like gravity.
    Eigen::VectorXd const specific_body_force;

    MeshLib::PropertyVector<double>* pressure_interpolated = nullptr;
};

}  // namespace StokesFlow
}  // namespace ProcessLib