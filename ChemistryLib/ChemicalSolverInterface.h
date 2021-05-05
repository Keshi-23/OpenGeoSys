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

#include "MaterialLib/MPL/VariableType.h"
#include "MathLib/LinAlg/GlobalMatrixVectorTypes.h"

namespace MaterialPropertyLib
{
class Medium;
}

namespace ParameterLib
{
class SpatialPosition;
}

namespace ChemistryLib
{
class ChemicalSolverInterface
{
public:
    virtual void initialize() {}

    virtual void initializeChemicalSystemConcrete(
        std::vector<double> const& /*concentrations*/,
        GlobalIndexType const& /*chemical_system_id*/,
        MaterialPropertyLib::Medium const& /*medium*/,
        ParameterLib::SpatialPosition const& /*pos*/, double const /*t*/)
    {
    }

    virtual void setChemicalSystemConcrete(
        std::vector<double> const& /*concentrations*/,
        GlobalIndexType const& /*chemical_system_id*/,
        MaterialPropertyLib::Medium const* /*medium*/,
        MaterialPropertyLib::VariableArray const& /*vars*/,
        ParameterLib::SpatialPosition const& /*pos*/, double const /*t*/,
        double const /*dt*/)
    {
    }

    virtual void setAqueousSolutionsPrevFromDumpFile() {}

    virtual void executeSpeciationCalculation(double const dt) = 0;

    virtual std::vector<GlobalVector*> getIntPtProcessSolutions() const = 0;

    virtual std::vector<std::string> const getComponentList() const
    {
        return {};
    }

    virtual void updateVolumeFractionPostReaction(
        GlobalIndexType const& /*chemical_system_id*/,
        MaterialPropertyLib::Medium const& /*medium*/,
        ParameterLib::SpatialPosition const& /*pos*/, double const /*porosity*/,
        double const /*t*/, double const /*dt*/)
    {
    }

    virtual void updatePorosityPostReaction(
        GlobalIndexType const& /*chemical_system_id*/,
        MaterialPropertyLib::Medium const& /*medium*/,
        double& /*porosity*/)
    {
    }

    virtual void computeSecondaryVariable(
        std::size_t const /*ele_id*/,
        std::vector<GlobalIndexType> const& /*chemical_system_indices*/)
    {
    }

    virtual ~ChemicalSolverInterface() = default;

public:
    std::vector<GlobalIndexType> chemical_system_index_map;
};
}  // namespace ChemistryLib
