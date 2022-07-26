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

#include <memory>
#include <vector>

#include "ConstitutiveSetting.h"
#include "IntegrationPointData.h"
#include "LocalAssemblerInterface.h"
#include "MathLib/KelvinVector.h"
#include "NumLib/Fem/InitShapeMatrices.h"
#include "NumLib/Fem/Integration/GenericIntegrationMethod.h"
#include "NumLib/Fem/ShapeMatrixPolicy.h"
#include "ProcessLib/Deformation/BMatrixPolicy.h"
#include "ThermoRichardsMechanicsProcessData.h"

namespace ProcessLib
{
namespace ThermoRichardsMechanics
{
namespace MPL = MaterialPropertyLib;

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
class ThermoRichardsMechanicsLocalAssembler
    : public LocalAssemblerInterface<DisplacementDim>
{
    static constexpr int temperature_index = 0;
    static constexpr int temperature_size = ShapeFunction::NPOINTS;
    static constexpr int pressure_index = temperature_size;
    static constexpr int pressure_size = ShapeFunction::NPOINTS;
    static constexpr int displacement_index = 2 * ShapeFunction::NPOINTS;
    static constexpr int displacement_size =
        ShapeFunctionDisplacement::NPOINTS * DisplacementDim;

public:
    using ShapeMatricesTypeDisplacement =
        ShapeMatrixPolicyType<ShapeFunctionDisplacement, DisplacementDim>;
    // Note: temperature variable uses the same shape functions as that are used
    // by pressure variable.
    using ShapeMatricesType =
        ShapeMatrixPolicyType<ShapeFunction, DisplacementDim>;

    using GlobalDimMatrixType = typename ShapeMatricesType::GlobalDimMatrixType;
    using GlobalDimVectorType = typename ShapeMatricesType::GlobalDimVectorType;

    using BMatricesType =
        BMatrixPolicyType<ShapeFunctionDisplacement, DisplacementDim>;
    using KelvinVectorType = typename BMatricesType::KelvinVectorType;

    using IpData = IntegrationPointData<ShapeMatricesTypeDisplacement,
                                        ShapeMatricesType, DisplacementDim,
                                        ShapeFunctionDisplacement::NPOINTS>;

    static int const KelvinVectorSize =
        MathLib::KelvinVector::kelvin_vector_dimensions(DisplacementDim);
    using Invariants = MathLib::KelvinVector::Invariants<KelvinVectorSize>;

    using SymmetricTensor = Eigen::Matrix<double, KelvinVectorSize, 1>;

    ThermoRichardsMechanicsLocalAssembler(
        ThermoRichardsMechanicsLocalAssembler const&) = delete;
    ThermoRichardsMechanicsLocalAssembler(
        ThermoRichardsMechanicsLocalAssembler&&) = delete;

    ThermoRichardsMechanicsLocalAssembler(
        MeshLib::Element const& e,
        std::size_t const /*local_matrix_size*/,
        NumLib::GenericIntegrationMethod const& integration_method,
        bool const is_axially_symmetric,
        ThermoRichardsMechanicsProcessData<DisplacementDim>& process_data);

    void setInitialConditionsConcrete(std::vector<double> const& local_x,
                                      double const t,
                                      bool const use_monolithic_scheme,
                                      int const process_id) override;

    class LocalMatrices
    {
        using NodalMatrix = typename ShapeMatricesType::NodalMatrixType;

        static auto constexpr local_matrix_dim =
            displacement_size + pressure_size + temperature_size;

        template <Eigen::Index rows, Eigen::Index cols>
        using Mat =
            typename ShapeMatricesTypeDisplacement::template MatrixType<rows,
                                                                        cols>;
        using Vec = typename ShapeMatricesTypeDisplacement::template VectorType<
            local_matrix_dim>;

    public:
        void setZero()
        {
            M_TT = NodalMatrix::Zero(temperature_size, temperature_size);
            M_Tp = NodalMatrix::Zero(temperature_size, pressure_size);
            K_TT = NodalMatrix::Zero(temperature_size, temperature_size);
            K_Tp = NodalMatrix::Zero(temperature_size, pressure_size);
            dK_TT_dp = NodalMatrix::Zero(temperature_size, pressure_size);

            M_pu = Mat<pressure_size, displacement_size>::Zero(
                pressure_size, displacement_size);

            M_pT = NodalMatrix::Zero(pressure_size, temperature_size);

            K_pp = NodalMatrix::Zero(pressure_size, pressure_size);
            K_pT = NodalMatrix::Zero(pressure_size, temperature_size);

            storage_p_a_p = NodalMatrix::Zero(pressure_size, pressure_size);
            storage_p_a_S_Jpp = NodalMatrix::Zero(pressure_size, pressure_size);
            storage_p_a_S = NodalMatrix::Zero(pressure_size, pressure_size);

            Jac = Mat<local_matrix_dim, local_matrix_dim>::Zero(
                local_matrix_dim, local_matrix_dim);
            res = Vec::Zero(local_matrix_dim);
        }

        LocalMatrices& operator+=(LocalMatrices const& other)
        {
            M_TT += other.M_TT;
            M_Tp += other.M_Tp;
            K_TT += other.K_TT;
            K_Tp += other.K_Tp;
            dK_TT_dp += other.dK_TT_dp;

            M_pu += other.M_pu;

            M_pT += other.M_pT;

            K_pp += other.K_pp;
            K_pT += other.K_pT;

            storage_p_a_p += other.storage_p_a_p;
            storage_p_a_S_Jpp += other.storage_p_a_S_Jpp;
            storage_p_a_S += other.storage_p_a_S;

            Jac += other.Jac;
            res += other.res;

            return *this;
        }

        LocalMatrices& operator*=(double const a)
        {
            M_TT *= a;
            M_Tp *= a;
            K_TT *= a;
            K_Tp *= a;
            dK_TT_dp *= a;

            M_pu *= a;

            M_pT *= a;

            K_pp *= a;
            K_pT *= a;

            storage_p_a_p *= a;
            storage_p_a_S_Jpp *= a;
            storage_p_a_S *= a;

            Jac *= a;
            res *= a;

            return *this;
        }

        NodalMatrix M_TT;
        NodalMatrix M_Tp;
        NodalMatrix K_TT;
        NodalMatrix K_Tp;
        NodalMatrix dK_TT_dp;

        Mat<pressure_size, displacement_size> M_pu;

        NodalMatrix M_pT;

        NodalMatrix K_pp;
        NodalMatrix K_pT;

        NodalMatrix storage_p_a_p;
        NodalMatrix storage_p_a_S_Jpp;
        NodalMatrix storage_p_a_S;

        //! "Direct" contributions to the Jacobian, without those from K and M
        //! matrices.
        Mat<local_matrix_dim, local_matrix_dim> Jac;

        //! "Direct" contributions to the residual, without those from K and M
        //! matrices.
        Vec res;
    };

    void assembleWithJacobian(double const t, double const dt,
                              std::vector<double> const& local_x,
                              std::vector<double> const& local_xdot,
                              std::vector<double>& /*local_M_data*/,
                              std::vector<double>& /*local_K_data*/,
                              std::vector<double>& local_rhs_data,
                              std::vector<double>& local_Jac_data) override;

private:
    void assembleWithJacobianSingleIP(
        double const t, double const dt,
        ParameterLib::SpatialPosition const& x_position,
        std::vector<double> const& local_x,
        std::vector<double> const& local_xdot, IpData const& ip_data,
        ConstitutiveSetting<DisplacementDim>& CS,
        MaterialPropertyLib::Medium& medium, LocalMatrices& out,
        StatefulData<DisplacementDim>& current_state,
        StatefulData<DisplacementDim> const& prev_state,
        MaterialStateData<DisplacementDim>& mat_state,
        OutputData<DisplacementDim>& output_data) const;

    void addToLocalMatrixData(double const dt,
                              std::vector<double> const& local_x,
                              std::vector<double> const& local_xdot,
                              LocalMatrices const& loc_mat,
                              std::vector<double>& local_rhs_data,
                              std::vector<double>& local_Jac_data) const;

    void massLumping(LocalMatrices& loc_mat) const;

    //! Makes local d.o.f.s more accessible.
    //! Order of the returned Eigen vectors: T, p_L, u.
    auto localDOF(std::vector<double> const& local_dof_data) const
    {
        static_assert(temperature_size == pressure_size);

        using NodalTOrPVec =
            typename ShapeMatricesType::template VectorType<temperature_size>;
        using NodalDispVec =
            typename ShapeMatricesTypeDisplacement::template VectorType<
                displacement_size>;

        return std::tuple<Eigen::Map<NodalTOrPVec const>,
                          Eigen::Map<NodalTOrPVec const>,
                          Eigen::Map<NodalDispVec const>>(
            {local_dof_data.data() + temperature_index, temperature_size},
            {local_dof_data.data() + pressure_index, pressure_size},
            {local_dof_data.data() + displacement_index, displacement_size});
    };

public:
    void initializeConcrete() override
    {
        unsigned const n_integration_points =
            this->integration_method_.getNumberOfPoints();

        for (unsigned ip = 0; ip < n_integration_points; ip++)
        {
            // Set initial stress from parameter.
            if (this->process_data_.initial_stress != nullptr)
            {
                ParameterLib::SpatialPosition const x_position{
                    std::nullopt, this->element_.getID(), ip,
                    MathLib::Point3d(NumLib::interpolateCoordinates<
                                     ShapeFunctionDisplacement,
                                     ShapeMatricesTypeDisplacement>(
                        this->element_, ip_data_[ip].N_u))};

                this->current_states_[ip].s_mech_data.sigma_eff =
                    MathLib::KelvinVector::symmetricTensorToKelvinVector<
                        DisplacementDim>((*this->process_data_.initial_stress)(
                        std::numeric_limits<
                            double>::quiet_NaN() /* time independent */,
                        x_position));
            }

            this->material_states_[ip].pushBackState();
        }

        this->prev_states_ = this->current_states_;
    }

    void computeSecondaryVariableConcrete(
        double const t, double const dt, Eigen::VectorXd const& local_x,
        Eigen::VectorXd const& local_x_dot) override;

    Eigen::Map<const Eigen::RowVectorXd> getShapeMatrix(
        const unsigned integration_point) const override
    {
        auto const& N_u = ip_data_[integration_point].N_u;

        // assumes N is stored contiguously in memory
        return Eigen::Map<const Eigen::RowVectorXd>(N_u.data(), N_u.size());
    }

private:
    std::vector<IpData> ip_data_;

    static auto block_uu(auto& mat)
    {
        return mat.template block<displacement_size, displacement_size>(
            displacement_index, displacement_index);
    }
    static auto block_up(auto& mat)
    {
        return mat.template block<displacement_size, pressure_size>(
            displacement_index, pressure_index);
    }
    static auto block_uT(auto& mat)
    {
        return mat.template block<displacement_size, temperature_size>(
            displacement_index, temperature_index);
    }
    static auto block_pu(auto& mat)
    {
        return mat.template block<pressure_size, displacement_size>(
            pressure_index, displacement_index);
    }
    static auto block_pp(auto& mat)
    {
        return mat.template block<pressure_size, pressure_size>(pressure_index,
                                                                pressure_index);
    }
    static auto block_pT(auto& mat)
    {
        return mat.template block<pressure_size, temperature_size>(
            pressure_index, temperature_index);
    }
    static auto block_Tp(auto& mat)
    {
        return mat.template block<temperature_size, pressure_size>(
            temperature_index, pressure_index);
    }
    static auto block_TT(auto& mat)
    {
        return mat.template block<temperature_size, temperature_size>(
            temperature_index, temperature_index);
    }

    static auto block_u(auto& vec)
    {
        return vec.template segment<displacement_size>(displacement_index);
    }
    static auto block_p(auto& vec)
    {
        return vec.template segment<pressure_size>(pressure_index);
    }
    static auto block_T(auto& vec)
    {
        return vec.template segment<temperature_size>(temperature_index);
    }
};

}  // namespace ThermoRichardsMechanics
}  // namespace ProcessLib

#include "ThermoRichardsMechanicsFEM-impl.h"
