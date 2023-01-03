/**
 * \file
 * \copyright
 * Copyright (c) 2012-2023, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 */

#pragma once

namespace BaseLib
{
class ConfigTree;
}

namespace NumLib
{
struct NewtonRaphsonSolverParameters;

NewtonRaphsonSolverParameters createNewtonRaphsonSolverParameters(
    BaseLib::ConfigTree const& config);
}  // namespace NumLib
