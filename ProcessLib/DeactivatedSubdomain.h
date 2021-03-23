/**
 * \file
 * \copyright
 * Copyright (c) 2012-2021, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 * File:   DeactivatedSubdomain.h
 *
 * Created on November 29, 2018, 10:50 AM
 */
#pragma once

#include <Eigen/Dense>
#include <memory>
#include <string>
#include <vector>

#include "MathLib/InterpolationAlgorithms/PiecewiseLinearInterpolation.h"

namespace MeshLib
{
class Mesh;
class Node;
}  // namespace MeshLib

namespace ProcessLib
{
struct DeactivatedSubdomainMesh
{
    DeactivatedSubdomainMesh(
        std::unique_ptr<MeshLib::Mesh> deactivated_subdomain_mesh_,
        std::vector<MeshLib::Node*>&& inner_nodes_);

    std::unique_ptr<MeshLib::Mesh> const mesh;
    std::vector<MeshLib::Node*> const inner_nodes;
};

struct DeactivatedSubdomain
{
    DeactivatedSubdomain(
        MathLib::PiecewiseLinearInterpolation time_interval_,
        std::pair<Eigen::Vector3d, Eigen::Vector3d>
            line_segment,
        std::vector<int>&& materialIDs_,
        std::vector<std::unique_ptr<DeactivatedSubdomainMesh>>&&
            deactivated_subdomain_meshes_);

    bool includesTimeOf(double const t) const;

    MathLib::PiecewiseLinearInterpolation const time_interval;

    /// Line segment along which excavation progresses. Represented by start and
    /// end points.
    std::pair<Eigen::Vector3d, Eigen::Vector3d> line_segment;

    /// The material IDs of the deactivated the subdomains
    std::vector<int> const materialIDs;

    std::vector<std::unique_ptr<DeactivatedSubdomainMesh>> const
        deactivated_subdomain_meshes;

    static const std::string zero_parameter_name;
};
}  // namespace ProcessLib
