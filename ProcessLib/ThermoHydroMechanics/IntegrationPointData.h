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

#include <memory>

#include "MaterialLib/SolidModels/LinearElasticIsotropic.h"
#include "MathLib/KelvinVector.h"
#include "MathLib/LinAlg/Eigen/EigenMapTools.h"
#include "ParameterLib/Parameter.h"

namespace ProcessLib
{
namespace ThermoHydroMechanics
{
template <typename BMatricesType, typename ShapeMatrixTypeDisplacement,
          typename ShapeMatricesTypePressure, int DisplacementDim, int NPoints>
struct IntegrationPointData final
{
    explicit IntegrationPointData(
        MaterialLib::Solids::MechanicsBase<DisplacementDim> const&
            solid_material)
        : solid_material(solid_material),
          material_state_variables(
              solid_material.createMaterialStateVariables())
    {
        // Initialize current time step values
        static const int kelvin_vector_size =
            MathLib::KelvinVector::kelvin_vector_dimensions(DisplacementDim);
        sigma_eff.setZero(kelvin_vector_size);
        eps.setZero(kelvin_vector_size);
        eps_prev.setZero(kelvin_vector_size);
        eps_m.setZero(kelvin_vector_size);
        eps_m_prev.resize(kelvin_vector_size);

        // Previous time step values are not initialized and are set later.
        sigma_eff_prev.resize(kelvin_vector_size);
    }

    typename ShapeMatrixTypeDisplacement::template MatrixType<
        DisplacementDim, NPoints * DisplacementDim>
        N_u_op;
    typename BMatricesType::KelvinVectorType sigma_eff, sigma_eff_prev;
    typename BMatricesType::KelvinVectorType eps, eps_prev;
    typename BMatricesType::KelvinVectorType eps_m, eps_m_prev;

    typename ShapeMatrixTypeDisplacement::NodalRowVectorType N_u;
    typename ShapeMatrixTypeDisplacement::GlobalDimNodalMatrixType dNdx_u;

    typename ShapeMatricesTypePressure::NodalRowVectorType N_p;
    typename ShapeMatricesTypePressure::GlobalDimNodalMatrixType dNdx_p;

    MaterialLib::Solids::MechanicsBase<DisplacementDim> const& solid_material;
    std::unique_ptr<typename MaterialLib::Solids::MechanicsBase<
        DisplacementDim>::MaterialStateVariables>
        material_state_variables;
    double integration_weight;

    void pushBackState()
    {
        eps_prev = eps;
        eps_m_prev = eps_m;
        sigma_eff_prev = sigma_eff;
        material_state_variables->pushBackState();
    }

    MathLib::KelvinVector::KelvinMatrixType<DisplacementDim>
    computeElasticTangentStiffness(
        double const t,
        ParameterLib::SpatialPosition const& x_position,
        double const dt,
        double const temperature)
    {
        namespace MPL = MaterialPropertyLib;

        MPL::VariableArray variable_array;
        MPL::VariableArray variable_array_prev;

        auto const null_state = solid_material.createMaterialStateVariables();

        using KV = MathLib::KelvinVector::KelvinVectorType<DisplacementDim>;

        variable_array[static_cast<int>(MPL::Variable::stress)].emplace<KV>(
            KV::Zero());
        variable_array[static_cast<int>(MPL::Variable::mechanical_strain)]
            .emplace<KV>(KV::Zero());
        variable_array[static_cast<int>(MPL::Variable::temperature)]
            .emplace<double>(temperature);

        variable_array_prev[static_cast<int>(MPL::Variable::stress)]
            .emplace<KV>(KV::Zero());
        variable_array_prev[static_cast<int>(MPL::Variable::mechanical_strain)]
            .emplace<KV>(KV::Zero());
        variable_array_prev[static_cast<int>(MPL::Variable::temperature)]
            .emplace<double>(temperature);

        auto&& solution =
            solid_material.integrateStress(variable_array_prev, variable_array,
                                           t, x_position, dt, *null_state);

        if (!solution)
        {
            OGS_FATAL("Computation of elastic tangent stiffness failed.");
        }

        MathLib::KelvinVector::KelvinMatrixType<DisplacementDim> C =
            std::move(std::get<2>(*solution));

        return C;
    }

    typename BMatricesType::KelvinMatrixType updateConstitutiveRelation(
        MaterialPropertyLib::VariableArray const& variable_array,
        double const t,
        ParameterLib::SpatialPosition const& x_position,
        double const dt,
        double const temperature_prev)
    {
        MaterialPropertyLib::VariableArray variable_array_prev;
        variable_array_prev[static_cast<int>(
                                MaterialPropertyLib::Variable::stress)]
            .emplace<MathLib::KelvinVector::KelvinVectorType<DisplacementDim>>(
                sigma_eff_prev);
        variable_array_prev[static_cast<int>(MaterialPropertyLib::Variable::
                                                 mechanical_strain)]
            .emplace<MathLib::KelvinVector::KelvinVectorType<DisplacementDim>>(
                eps_m_prev);
        variable_array_prev[static_cast<int>(
                                MaterialPropertyLib::Variable::temperature)]
            .emplace<double>(temperature_prev);

        auto&& solution = solid_material.integrateStress(
            variable_array_prev, variable_array, t, x_position, dt,
            *material_state_variables);

        if (!solution)
            OGS_FATAL("Computation of local constitutive relation failed.");

        MathLib::KelvinVector::KelvinMatrixType<DisplacementDim> C;
        std::tie(sigma_eff, material_state_variables, C) = std::move(*solution);

        return C;
    }

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
};

}  // namespace ThermoHydroMechanics
}  // namespace ProcessLib
