/**
 * \copyright
 * Copyright (c) 2012-2022, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 *  \file
 *  Created on November 29, 2017, 2:03 PM
 */

#pragma once

#include <spdlog/fmt/ostr.h>

#include <cassert>

#include "ConstitutiveSetting.h"
#include "MaterialLib/MPL/Medium.h"
#include "MaterialLib/MPL/Utils/FormEigenTensor.h"
#include "MaterialLib/MPL/Utils/FormKelvinVectorFromThermalExpansivity.h"
#include "MaterialLib/MPL/Utils/GetLiquidThermalExpansivity.h"
#include "MaterialLib/PhysicalConstant.h"
#include "MaterialLib/SolidModels/SelectSolidConstitutiveRelation.h"
#include "MathLib/KelvinVector.h"
#include "NumLib/Function/Interpolation.h"
#include "ProcessLib/Utils/SetOrGetIntegrationPointData.h"
#include "ProcessLib/Utils/TransposeInPlace.h"
#include "ThermoRichardsMechanicsFEM.h"

namespace ProcessLib
{
namespace ThermoRichardsMechanics
{
template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
ThermoRichardsMechanicsLocalAssembler<ShapeFunctionDisplacement, ShapeFunction,
                                      DisplacementDim>::
    ThermoRichardsMechanicsLocalAssembler(
        MeshLib::Element const& e,
        std::size_t const /*local_matrix_size*/,
        NumLib::GenericIntegrationMethod const& integration_method,
        bool const is_axially_symmetric,
        ThermoRichardsMechanicsProcessData<DisplacementDim>& process_data)
    : process_data_(process_data),
      integration_method_(integration_method),
      element_(e),
      is_axially_symmetric_(is_axially_symmetric),
      solid_material_(MaterialLib::Solids::selectSolidConstitutiveRelation(
          process_data_.solid_materials, process_data_.material_ids, e.getID()))
{
    unsigned const n_integration_points =
        integration_method_.getNumberOfPoints();

    current_states_.resize(n_integration_points);
    prev_states_.resize(n_integration_points);
    ip_data_.resize(n_integration_points);
    secondary_data_.N_u.resize(n_integration_points);
    output_data_.resize(n_integration_points);

    material_states_.reserve(n_integration_points);
    for (unsigned ip = 0; ip < n_integration_points; ++ip)
    {
        material_states_.emplace_back(solid_material_);
    }

    auto const shape_matrices_u =
        NumLib::initShapeMatrices<ShapeFunctionDisplacement,
                                  ShapeMatricesTypeDisplacement,
                                  DisplacementDim>(e, is_axially_symmetric,
                                                   integration_method_);

    auto const shape_matrices =
        NumLib::initShapeMatrices<ShapeFunction, ShapeMatricesType,
                                  DisplacementDim>(e, is_axially_symmetric,
                                                   integration_method_);

    auto const& medium = process_data_.media_map->getMedium(element_.getID());

    ParameterLib::SpatialPosition x_position;
    x_position.setElementID(element_.getID());
    for (unsigned ip = 0; ip < n_integration_points; ip++)
    {
        x_position.setIntegrationPoint(ip);
        auto& ip_data = ip_data_[ip];
        auto const& sm_u = shape_matrices_u[ip];
        ip_data_[ip].integration_weight =
            integration_method_.getWeightedPoint(ip).getWeight() *
            sm_u.integralMeasure * sm_u.detJ;

        ip_data.N_u_op = ShapeMatricesTypeDisplacement::template MatrixType<
            DisplacementDim, displacement_size>::Zero(DisplacementDim,
                                                      displacement_size);
        for (int i = 0; i < DisplacementDim; ++i)
        {
            ip_data.N_u_op
                .template block<1, displacement_size / DisplacementDim>(
                    i, i * displacement_size / DisplacementDim)
                .noalias() = sm_u.N;
        }

        ip_data.N_u = sm_u.N;
        ip_data.dNdx_u = sm_u.dNdx;

        // ip_data.N_p and ip_data.dNdx_p are used for both p and T variables
        ip_data.N_p = shape_matrices[ip].N;
        ip_data.dNdx_p = shape_matrices[ip].dNdx;

        auto& current_state = current_states_[ip];

        // Initial porosity. Could be read from integration point data or mesh.
        current_state.poro_data.phi =
            medium->property(MPL::porosity)
                .template initialValue<double>(
                    x_position,
                    std::numeric_limits<
                        double>::quiet_NaN() /* t independent */);

        if (medium->hasProperty(MPL::PropertyType::transport_porosity))
        {
            current_state.transport_poro_data.phi =
                medium->property(MPL::transport_porosity)
                    .template initialValue<double>(
                        x_position,
                        std::numeric_limits<
                            double>::quiet_NaN() /* t independent */);
        }
        else
        {
            current_state.transport_poro_data.phi = current_state.poro_data.phi;
        }

        secondary_data_.N_u[ip] = shape_matrices_u[ip].N;
    }
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::size_t ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction,
    DisplacementDim>::setIPDataInitialConditions(std::string const& name,
                                                 double const* values,
                                                 int const integration_order)
{
    if (integration_order !=
        static_cast<int>(integration_method_.getIntegrationOrder()))
    {
        OGS_FATAL(
            "Setting integration point initial conditions; The integration "
            "order of the local assembler for element {:d} is different "
            "from the integration order in the initial condition.",
            element_.getID());
    }

    if (name == "sigma_ip")
    {
        if (process_data_.initial_stress != nullptr)
        {
            OGS_FATAL(
                "Setting initial conditions for stress from integration "
                "point data and from a parameter '{:s}' is not possible "
                "simultaneously.",
                process_data_.initial_stress->name);
        }
        return ProcessLib::setIntegrationPointKelvinVectorData<DisplacementDim>(
            values, current_states_,
            [](auto& cs) -> auto& { return cs.s_mech_data.sigma_eff; });
    }

    if (name == "saturation_ip")
    {
        return ProcessLib::setIntegrationPointScalarData(
            values, current_states_,
            [](auto& state) -> auto& { return state.S_L_data.S_L; });
    }
    if (name == "porosity_ip")
    {
        return ProcessLib::setIntegrationPointScalarData(
            values, current_states_,
            [](auto& state) -> auto& { return state.poro_data.phi; });
    }
    if (name == "transport_porosity_ip")
    {
        return ProcessLib::setIntegrationPointScalarData(
            values, current_states_,
            [](auto& state) -> auto& { return state.transport_poro_data.phi; });
    }
    if (name == "swelling_stress_ip")
    {
        return ProcessLib::setIntegrationPointKelvinVectorData<DisplacementDim>(
            values, current_states_,
            [](auto& cs) -> auto& { return cs.swelling_data.sigma_sw; });
    }
    if (name == "epsilon_ip")
    {
        return ProcessLib::setIntegrationPointKelvinVectorData<DisplacementDim>(
            values, current_states_,
            [](auto& cs) -> auto& { return cs.eps_data.eps; });
    }
    return 0;
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
void ThermoRichardsMechanicsLocalAssembler<ShapeFunctionDisplacement,
                                           ShapeFunction, DisplacementDim>::
    setInitialConditionsConcrete(std::vector<double> const& local_x,
                                 double const t,
                                 bool const /*use_monolithic_scheme*/,
                                 int const /*process_id*/)
{
    assert(local_x.size() ==
           temperature_size + pressure_size + displacement_size);

    auto const p_L = Eigen::Map<
        typename ShapeMatricesType::template VectorType<pressure_size> const>(
        local_x.data() + pressure_index, pressure_size);

    auto const T = Eigen::Map<typename ShapeMatricesType::template VectorType<
        temperature_size> const>(local_x.data() + temperature_index,
                                 temperature_size);

    constexpr double dt = std::numeric_limits<double>::quiet_NaN();
    auto const& medium = process_data_.media_map->getMedium(element_.getID());
    MPL::VariableArray variables;

    ParameterLib::SpatialPosition x_position;
    x_position.setElementID(element_.getID());

    auto const& solid_phase = medium->phase("Solid");

    unsigned const n_integration_points =
        integration_method_.getNumberOfPoints();
    for (unsigned ip = 0; ip < n_integration_points; ip++)
    {
        x_position.setIntegrationPoint(ip);

        // N is used for both T and p variables.
        auto const& N = ip_data_[ip].N_p;

        double p_cap_ip;
        NumLib::shapeFunctionInterpolate(-p_L, N, p_cap_ip);

        variables[static_cast<int>(MPL::Variable::capillary_pressure)] =
            p_cap_ip;
        variables[static_cast<int>(MPL::Variable::phase_pressure)] = -p_cap_ip;

        double T_ip;
        NumLib::shapeFunctionInterpolate(T, N, T_ip);
        variables[static_cast<int>(MPL::Variable::temperature)] = T_ip;

        prev_states_[ip].S_L_data.S_L =
            medium->property(MPL::PropertyType::saturation)
                .template value<double>(variables, x_position, t, dt);

        // Set eps_m_prev from potentially non-zero eps and sigma_sw from
        // restart.
        SpaceTimeData const x_t{x_position, t, dt};
        ElasticTangentStiffnessData<DisplacementDim> C_el_data;
        ElasticTangentStiffnessModel<DisplacementDim>{solid_material_}.eval(
            x_t, {T_ip, 0, {}}, C_el_data);

        auto const& eps = current_states_[ip].eps_data.eps;
        auto const& sigma_sw = current_states_[ip].swelling_data.sigma_sw;
        prev_states_[ip].s_mech_data.eps_m.noalias() =
            solid_phase.hasProperty(MPL::PropertyType::swelling_stress_rate)
                ? eps + C_el_data.C_el.inverse() * sigma_sw
                : eps;
    }
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
void ThermoRichardsMechanicsLocalAssembler<ShapeFunctionDisplacement,
                                           ShapeFunction, DisplacementDim>::
    assembleWithJacobian(double const t, double const dt,
                         std::vector<double> const& local_x,
                         std::vector<double> const& local_xdot,
                         std::vector<double>& /*local_M_data*/,
                         std::vector<double>& /*local_K_data*/,
                         std::vector<double>& local_rhs_data,
                         std::vector<double>& local_Jac_data)
{
    auto& medium = *process_data_.media_map->getMedium(element_.getID());

    ParameterLib::SpatialPosition x_position;
    x_position.setElementID(element_.getID());

    LocalMatrices loc_mat;
    loc_mat.setZero();
    LocalMatrices loc_mat_current_ip;
    loc_mat_current_ip.setZero();  // only to set the right matrix sizes

    ConstitutiveSetting<DisplacementDim> constitutive_setting(solid_material_,
                                                              process_data_);

    for (unsigned ip = 0; ip < integration_method_.getNumberOfPoints(); ++ip)
    {
        x_position.setIntegrationPoint(ip);

        assembleWithJacobianSingleIP(t, dt, x_position,    //
                                     local_x, local_xdot,  //
                                     ip_data_[ip], constitutive_setting,
                                     medium,              //
                                     loc_mat_current_ip,  //
                                     current_states_[ip], prev_states_[ip],
                                     material_states_[ip], output_data_[ip]);
        loc_mat += loc_mat_current_ip;
    }

    massLumping(loc_mat);

    addToLocalMatrixData(dt, local_x, local_xdot, loc_mat, local_rhs_data,
                         local_Jac_data);
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
void ThermoRichardsMechanicsLocalAssembler<ShapeFunctionDisplacement,
                                           ShapeFunction, DisplacementDim>::
    massLumping(typename ThermoRichardsMechanicsLocalAssembler<
                ShapeFunctionDisplacement, ShapeFunction,
                DisplacementDim>::LocalMatrices& loc_mat) const
{
    if (process_data_.apply_mass_lumping)
    {
        loc_mat.storage_p_a_p =
            loc_mat.storage_p_a_p.colwise().sum().eval().asDiagonal();
        loc_mat.storage_p_a_S =
            loc_mat.storage_p_a_S.colwise().sum().eval().asDiagonal();
        loc_mat.storage_p_a_S_Jpp =
            loc_mat.storage_p_a_S_Jpp.colwise().sum().eval().asDiagonal();
    }
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
void ThermoRichardsMechanicsLocalAssembler<ShapeFunctionDisplacement,
                                           ShapeFunction, DisplacementDim>::
    addToLocalMatrixData(double const dt,
                         std::vector<double> const& local_x,
                         std::vector<double> const& local_xdot,
                         typename ThermoRichardsMechanicsLocalAssembler<
                             ShapeFunctionDisplacement, ShapeFunction,
                             DisplacementDim>::LocalMatrices const& loc_mat,
                         std::vector<double>& local_rhs_data,
                         std::vector<double>& local_Jac_data) const
{
    constexpr auto local_matrix_dim =
        displacement_size + pressure_size + temperature_size;
    assert(local_x.size() == local_matrix_dim);

    auto local_Jac = MathLib::createZeroedMatrix<
        typename ShapeMatricesTypeDisplacement::template MatrixType<
            local_matrix_dim, local_matrix_dim>>(
        local_Jac_data, local_matrix_dim, local_matrix_dim);

    auto local_rhs =
        MathLib::createZeroedVector<typename ShapeMatricesTypeDisplacement::
                                        template VectorType<local_matrix_dim>>(
            local_rhs_data, local_matrix_dim);

    local_Jac.noalias() = loc_mat.Jac;
    local_rhs.noalias() = -loc_mat.res;

    //
    // -- Jacobian
    //
    block_TT(local_Jac).noalias() += loc_mat.M_TT / dt + loc_mat.K_TT;
    block_Tp(local_Jac).noalias() +=
        loc_mat.K_Tp + loc_mat.dK_TT_dp + loc_mat.M_Tp / dt;

    block_pT(local_Jac).noalias() += loc_mat.M_pT / dt + loc_mat.K_pT;
    block_pp(local_Jac).noalias() +=
        loc_mat.K_pp + loc_mat.storage_p_a_p / dt + loc_mat.storage_p_a_S_Jpp;
    block_pu(local_Jac).noalias() = loc_mat.M_pu / dt;

    //
    // -- Residual
    //
    auto const [T, p_L, u] = localDOF(local_x);
    auto const [T_dot, p_L_dot, u_dot] = localDOF(local_xdot);

    block_T(local_rhs).noalias() -= loc_mat.M_TT * T_dot + loc_mat.K_TT * T +
                                    loc_mat.K_Tp * p_L + loc_mat.M_Tp * p_L_dot;
    block_p(local_rhs).noalias() -=
        loc_mat.K_pp * p_L + loc_mat.K_pT * T +
        (loc_mat.storage_p_a_p + loc_mat.storage_p_a_S) * p_L_dot +
        loc_mat.M_pu * u_dot + loc_mat.M_pT * T_dot;
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
void ThermoRichardsMechanicsLocalAssembler<ShapeFunctionDisplacement,
                                           ShapeFunction, DisplacementDim>::
    assembleWithJacobianSingleIP(
        double const t, double const dt,
        ParameterLib::SpatialPosition const& x_position,
        std::vector<double> const& local_x,
        std::vector<double> const& local_xdot,
        typename ThermoRichardsMechanicsLocalAssembler<
            ShapeFunctionDisplacement, ShapeFunction,
            DisplacementDim>::IpData const& ip_data,
        ConstitutiveSetting<DisplacementDim>& CS,
        MaterialPropertyLib::Medium& medium,
        typename ThermoRichardsMechanicsLocalAssembler<
            ShapeFunctionDisplacement, ShapeFunction,
            DisplacementDim>::LocalMatrices& out,
        StatefulData<DisplacementDim>& current_state,
        StatefulData<DisplacementDim> const& prev_state,
        MaterialStateData<DisplacementDim>& mat_state,
        OutputData<DisplacementDim>& output_data) const
{
    auto const& N_u = ip_data.N_u;
    auto const& N_u_op = ip_data.N_u_op;
    auto const& dNdx_u = ip_data.dNdx_u;

    // N and dNdx are used for both p and T variables
    auto const& N = ip_data.N_p;
    auto const& dNdx = ip_data.dNdx_p;

    auto const x_coord =
        NumLib::interpolateXCoordinate<ShapeFunctionDisplacement,
                                       ShapeMatricesTypeDisplacement>(element_,
                                                                      N_u);
    auto const B =
        LinearBMatrix::computeBMatrix<DisplacementDim,
                                      ShapeFunctionDisplacement::NPOINTS,
                                      typename BMatricesType::BMatrixType>(
            dNdx_u, N_u, x_coord, is_axially_symmetric_);

    auto const [T, p_L, u] = localDOF(local_x);
    auto const [T_dot, p_L_dot, u_dot] = localDOF(local_xdot);

    GlobalDimVectorType const grad_T_ip = dNdx * T;

    ConstitutiveModels<DisplacementDim> models(process_data_, solid_material_);
    ConstitutiveTempData<DisplacementDim> tmp;
    ConstitutiveData<DisplacementDim> CD;

    {
        double const T_ip = N * T;
        double const T_dot_ip = N * T_dot;

        double const p_cap_ip = -N * p_L;
        double const p_cap_dot_ip = -N * p_L_dot;
        GlobalDimVectorType const grad_p_cap_ip = -dNdx * p_L;

        KelvinVectorType eps = B * u;
        // TODO conceptual consistency check. introduced for volumetric strain
        // rate computation
        KelvinVectorType eps_prev = eps - B * (u_dot * dt);

        CS.eval(models, t, dt, x_position,                //
                medium,                                   //
                {T_ip, T_dot_ip, grad_T_ip},              //
                {p_cap_ip, p_cap_dot_ip, grad_p_cap_ip},  //
                eps, eps_prev, current_state, prev_state, mat_state, tmp,
                output_data, CD);
    }

    using NodalMatrix = typename ShapeMatricesType::NodalMatrixType;
    NodalMatrix const NTN = N.transpose() * N;
    NodalMatrix const dNTdN = dNdx.transpose() * dNdx;

    // TODO is identity2.transpose() * B something like divergence?
    auto const& identity2 = MathLib::KelvinVector::Invariants<
        MathLib::KelvinVector::kelvin_vector_dimensions(
            DisplacementDim)>::identity2;
    typename ShapeMatricesTypeDisplacement::template MatrixType<
        displacement_size, pressure_size>
        BTI2N = B.transpose() * identity2 * N;

    /*
     * Conventions:
     *
     * * use positive signs exclusively, any negative signs must be included in
     *   the coefficients coming from the constitutive setting
     * * the used coefficients from the constitutive setting are named after the
     *   terms they appear in, e.g. K_TT_X_dNTdN means it is a scalar (X) that
     *   will be multiplied by dNdx.transpose() * dNdx. Placefolders for the
     *   coefficients are:
     *     * X -> scalar
     *     * V -> vector
     *     * K -> Kelvin vector
     * * the Laplace terms have a special name, e.g., K_TT_Laplace
     * * there shall be only one contribution to each of the LocalMatrices,
     *   assigned with = assignment; this point might be relaxed in the future
     * * this method will overwrite the data in the passed LocalMatrices& out
     *   argument, not add to it
     */

    // residual, order T, p, u
    block_p(out.res).noalias() = dNdx.transpose() * CD.eq_p_data.rhs_p_dNT_V;
    block_u(out.res).noalias() =
        B.transpose() * CD.s_mech_data.sigma_total -
        N_u_op.transpose() * CD.grav_data.volumetric_body_force;

    // Storage matrices
    out.storage_p_a_p.noalias() = CD.eq_p_data.storage_p_a_p_X_NTN * NTN;
    out.storage_p_a_S.noalias() = CD.storage_data.storage_p_a_S_X_NTN * NTN;
    out.storage_p_a_S_Jpp.noalias() =
        CD.storage_data.storage_p_a_S_Jpp_X_NTN * NTN;

    // M matrices, order T, p, u
    out.M_TT.noalias() = CD.eq_T_data.M_TT_X_NTN * NTN;
    out.M_Tp.noalias() = CD.vap_data.M_Tp_X_NTN * NTN;

    out.M_pT.noalias() = CD.eq_p_data.M_pT_X_NTN * NTN;
    out.M_pu.noalias() = CD.eq_p_data.M_pu_X_BTI2N * BTI2N.transpose();

    // K matrices, order T, p, u
    out.K_TT.noalias() =
        dNdx.transpose() * CD.heat_data.K_TT_Laplace * dNdx +
        N.transpose() * (CD.eq_T_data.K_TT_NT_V_dN.transpose() * dNdx) +
        CD.vap_data.K_TT_X_dNTdN * dNTdN;

    out.dK_TT_dp.noalias() =
        N.transpose() * (CD.heat_data.K_Tp_NT_V_dN.transpose() * dNdx) +
        CD.heat_data.K_Tp_X_NTN * NTN + CD.vap_data.K_Tp_X_dNTdN * dNTdN;
    out.K_Tp.noalias() =
        dNdx.transpose() * CD.th_osmosis_data.K_Tp_Laplace * dNdx;

    out.K_pp.noalias() = dNdx.transpose() * CD.eq_p_data.K_pp_Laplace * dNdx +
                         CD.vap_data.K_pp_X_dNTdN * dNTdN;
    out.K_pT.noalias() =
        dNdx.transpose() * CD.th_osmosis_data.K_pT_Laplace * dNdx;

    // direct Jacobian contributions, order T, p, u
    block_pT(out.Jac).noalias() = CD.vap_data.J_pT_X_dNTdN * dNTdN;
    block_pp(out.Jac).noalias() =
        CD.storage_data.J_pp_X_NTN * NTN +
        CD.eq_p_data.J_pp_X_BTI2NT_u_dot_N * BTI2N.transpose() * u_dot *
            N  // TODO something with volumetric strain rate?
        + dNdx.transpose() * CD.eq_p_data.J_pp_dNT_V_N * N;

    block_uT(out.Jac).noalias() =
        B.transpose() * CD.s_mech_data.J_uT_BT_K_N * N;
    block_up(out.Jac).noalias() =
        B.transpose() * CD.swelling_data.J_up_BT_K_N * N +
        CD.eq_u_data.J_up_X_BTI2N * BTI2N +
        N_u_op.transpose() * CD.eq_u_data.J_up_HT_V_N * N;
    block_uu(out.Jac).noalias() =
        B.transpose() * CD.s_mech_data.stiffness_tensor * B;

    out *= ip_data.integration_weight;
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double> ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction, DisplacementDim>::getSigma() const
{
    constexpr int kelvin_vector_size =
        MathLib::KelvinVector::kelvin_vector_dimensions(DisplacementDim);

    return transposeInPlace<kelvin_vector_size>(
        [this](std::vector<double>& values)
        { return getIntPtSigma(0, {}, {}, values); });
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double> const& ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction, DisplacementDim>::
    getIntPtSigma(
        const double /*t*/,
        std::vector<GlobalVector*> const& /*x*/,
        std::vector<NumLib::LocalToGlobalIndexMap const*> const& /*dof_table*/,
        std::vector<double>& cache) const
{
    return ProcessLib::getIntegrationPointKelvinVectorData<DisplacementDim>(
        current_states_,
        [](auto const& cs) -> auto const& { return cs.s_mech_data.sigma_eff; },
        cache);
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double> ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction,
    DisplacementDim>::getSwellingStress() const
{
    constexpr int kelvin_vector_size =
        MathLib::KelvinVector::kelvin_vector_dimensions(DisplacementDim);

    return transposeInPlace<kelvin_vector_size>(
        [this](std::vector<double>& values)
        { return getIntPtSwellingStress(0, {}, {}, values); });
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double> const& ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction, DisplacementDim>::
    getIntPtSwellingStress(
        const double /*t*/,
        std::vector<GlobalVector*> const& /*x*/,
        std::vector<NumLib::LocalToGlobalIndexMap const*> const& /*dof_table*/,
        std::vector<double>& cache) const
{
    constexpr int kelvin_vector_size =
        MathLib::KelvinVector::kelvin_vector_dimensions(DisplacementDim);
    auto const n_integration_points = ip_data_.size();

    cache.clear();
    auto cache_mat = MathLib::createZeroedMatrix<Eigen::Matrix<
        double, kelvin_vector_size, Eigen::Dynamic, Eigen::RowMajor>>(
        cache, kelvin_vector_size, n_integration_points);

    for (unsigned ip = 0; ip < n_integration_points; ++ip)
    {
        auto const& sigma_sw = current_states_[ip].swelling_data.sigma_sw;
        cache_mat.col(ip) =
            MathLib::KelvinVector::kelvinVectorToSymmetricTensor(sigma_sw);
    }

    return cache;
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double>
ThermoRichardsMechanicsLocalAssembler<ShapeFunctionDisplacement, ShapeFunction,
                                      DisplacementDim>::getEpsilon() const
{
    constexpr int kelvin_vector_size =
        MathLib::KelvinVector::kelvin_vector_dimensions(DisplacementDim);

    return transposeInPlace<kelvin_vector_size>(
        [this](std::vector<double>& values)
        { return getIntPtEpsilon(0, {}, {}, values); });
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double> const& ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction, DisplacementDim>::
    getIntPtEpsilon(
        const double /*t*/,
        std::vector<GlobalVector*> const& /*x*/,
        std::vector<NumLib::LocalToGlobalIndexMap const*> const& /*dof_table*/,
        std::vector<double>& cache) const
{
    return ProcessLib::getIntegrationPointKelvinVectorData<DisplacementDim>(
        current_states_,
        [](auto const& cs) -> auto const& { return cs.eps_data.eps; }, cache);
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double> const& ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction, DisplacementDim>::
    getIntPtDarcyVelocity(
        const double /*t*/,
        std::vector<GlobalVector*> const& /*x*/,
        std::vector<NumLib::LocalToGlobalIndexMap const*> const& /*dof_table*/,
        std::vector<double>& cache) const
{
    unsigned const n_integration_points =
        integration_method_.getNumberOfPoints();

    cache.clear();
    auto cache_matrix = MathLib::createZeroedMatrix<Eigen::Matrix<
        double, DisplacementDim, Eigen::Dynamic, Eigen::RowMajor>>(
        cache, DisplacementDim, n_integration_points);

    for (unsigned ip = 0; ip < n_integration_points; ip++)
    {
        cache_matrix.col(ip).noalias() = output_data_[ip].darcy_data.v_darcy;
    }

    return cache;
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double>
ThermoRichardsMechanicsLocalAssembler<ShapeFunctionDisplacement, ShapeFunction,
                                      DisplacementDim>::getSaturation() const
{
    std::vector<double> result;
    getIntPtSaturation(0, {}, {}, result);
    return result;
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double> const& ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction, DisplacementDim>::
    getIntPtSaturation(
        const double /*t*/,
        std::vector<GlobalVector*> const& /*x*/,
        std::vector<NumLib::LocalToGlobalIndexMap const*> const& /*dof_table*/,
        std::vector<double>& cache) const
{
    return ProcessLib::getIntegrationPointScalarData(
        current_states_,
        [](auto const& state) -> auto const& { return state.S_L_data.S_L; },
        cache);
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double>
ThermoRichardsMechanicsLocalAssembler<ShapeFunctionDisplacement, ShapeFunction,
                                      DisplacementDim>::getPorosity() const
{
    std::vector<double> result;
    getIntPtPorosity(0, {}, {}, result);
    return result;
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double> const& ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction, DisplacementDim>::
    getIntPtPorosity(
        const double /*t*/,
        std::vector<GlobalVector*> const& /*x*/,
        std::vector<NumLib::LocalToGlobalIndexMap const*> const& /*dof_table*/,
        std::vector<double>& cache) const
{
    return ProcessLib::getIntegrationPointScalarData(
        current_states_,
        [](auto const& state) -> auto const& { return state.poro_data.phi; },
        cache);
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double> ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction,
    DisplacementDim>::getTransportPorosity() const
{
    std::vector<double> result;
    getIntPtTransportPorosity(0, {}, {}, result);
    return result;
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double> const& ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction, DisplacementDim>::
    getIntPtTransportPorosity(
        const double /*t*/,
        std::vector<GlobalVector*> const& /*x*/,
        std::vector<NumLib::LocalToGlobalIndexMap const*> const& /*dof_table*/,
        std::vector<double>& cache) const
{
    return ProcessLib::getIntegrationPointScalarData(
        current_states_, [](auto const& state) -> auto const& {
            return state.transport_poro_data.phi;
        },
        cache);
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double> const& ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction, DisplacementDim>::
    getIntPtDryDensitySolid(
        const double /*t*/,
        std::vector<GlobalVector*> const& /*x*/,
        std::vector<NumLib::LocalToGlobalIndexMap const*> const& /*dof_table*/,
        std::vector<double>& cache) const
{
    return ProcessLib::getIntegrationPointScalarData(
        output_data_,
        [](auto const& out) -> auto const& {
            return out.rho_S_data.dry_density_solid;
        },
        cache);
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double> const& ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction, DisplacementDim>::
    getIntPtLiquidDensity(
        const double /*t*/,
        std::vector<GlobalVector*> const& /*x*/,
        std::vector<NumLib::LocalToGlobalIndexMap const*> const& /*dof_table*/,
        std::vector<double>& cache) const
{
    return ProcessLib::getIntegrationPointScalarData(
        output_data_,
        [](auto const& out) -> auto const& { return out.rho_L_data.rho_LR; },
        cache);
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
std::vector<double> const& ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction, DisplacementDim>::
    getIntPtViscosity(
        const double /*t*/,
        std::vector<GlobalVector*> const& /*x*/,
        std::vector<NumLib::LocalToGlobalIndexMap const*> const& /*dof_table*/,
        std::vector<double>& cache) const
{
    return ProcessLib::getIntegrationPointScalarData(
        output_data_,
        [](auto const& out) -> auto const& { return out.mu_L_data.viscosity; },
        cache);
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
void ThermoRichardsMechanicsLocalAssembler<ShapeFunctionDisplacement,
                                           ShapeFunction, DisplacementDim>::
    computeSecondaryVariableConcrete(double const t, double const dt,
                                     Eigen::VectorXd const& local_x,
                                     Eigen::VectorXd const& local_x_dot)
{
    auto const T = block_T(local_x);
    auto const p_L = block_p(local_x);
    auto const u = block_u(local_x);

    auto const T_dot = block_T(local_x_dot);
    auto const p_L_dot = block_p(local_x_dot);
    auto const u_dot = block_u(local_x_dot);

    auto const e_id = element_.getID();
    auto& medium = *process_data_.media_map->getMedium(e_id);

    ParameterLib::SpatialPosition x_position;
    x_position.setElementID(e_id);

    unsigned const n_integration_points =
        integration_method_.getNumberOfPoints();

    double saturation_avg = 0;
    double porosity_avg = 0;
    double liquid_density_avg = 0;
    double viscosity_avg = 0;

    using KV = MathLib::KelvinVector::KelvinVectorType<DisplacementDim>;
    KV sigma_avg = KV::Zero();

    ConstitutiveSetting<DisplacementDim> constitutive_setting(solid_material_,
                                                              process_data_);

    ConstitutiveModels<DisplacementDim> models(process_data_, solid_material_);
    ConstitutiveTempData<DisplacementDim> tmp;
    ConstitutiveData<DisplacementDim> CD;

    for (unsigned ip = 0; ip < n_integration_points; ip++)
    {
        x_position.setIntegrationPoint(ip);

        auto const& ip_data = ip_data_[ip];

        // N is used for both p and T variables
        auto const& N = ip_data.N_p;
        auto const& N_u = ip_data.N_u;
        auto const& dNdx_u = ip_data.dNdx_u;
        auto const& dNdx = ip_data.dNdx_p;

        auto const x_coord =
            NumLib::interpolateXCoordinate<ShapeFunctionDisplacement,
                                           ShapeMatricesTypeDisplacement>(
                element_, N_u);
        auto const B =
            LinearBMatrix::computeBMatrix<DisplacementDim,
                                          ShapeFunctionDisplacement::NPOINTS,
                                          typename BMatricesType::BMatrixType>(
                dNdx_u, N_u, x_coord, is_axially_symmetric_);

        double const T_ip = N * T;
        double const T_dot_ip = N * T_dot;
        GlobalDimVectorType const grad_T_ip = dNdx * T;

        double const p_cap_ip = -N * p_L;
        double const p_cap_dot_ip = -N * p_L_dot;
        GlobalDimVectorType const grad_p_cap_ip = -dNdx * p_L;

        KelvinVectorType eps = B * u;
        // TODO conceptual consistency check. introduced for volumetric strain
        // rate computation
        KelvinVectorType eps_prev = eps - B * (u_dot * dt);

        constitutive_setting.eval(models,                                   //
                                  t, dt, x_position,                        //
                                  medium,                                   //
                                  {T_ip, T_dot_ip, grad_T_ip},              //
                                  {p_cap_ip, p_cap_dot_ip, grad_p_cap_ip},  //
                                  eps, eps_prev, current_states_[ip],
                                  prev_states_[ip], material_states_[ip], tmp,
                                  output_data_[ip], CD);

        saturation_avg += current_states_[ip].S_L_data.S_L;
        porosity_avg += current_states_[ip].poro_data.phi;

        liquid_density_avg += output_data_[ip].rho_L_data.rho_LR;
        viscosity_avg += output_data_[ip].mu_L_data.viscosity;
        sigma_avg += current_states_[ip].s_mech_data.sigma_eff;
    }
    saturation_avg /= n_integration_points;
    porosity_avg /= n_integration_points;
    viscosity_avg /= n_integration_points;
    liquid_density_avg /= n_integration_points;
    sigma_avg /= n_integration_points;

    (*process_data_.element_saturation)[e_id] = saturation_avg;
    (*process_data_.element_porosity)[e_id] = porosity_avg;
    (*process_data_.element_liquid_density)[e_id] = liquid_density_avg;
    (*process_data_.element_viscosity)[e_id] = viscosity_avg;

    Eigen::Map<KV>(
        &(*process_data_.element_stresses)[e_id * KV::RowsAtCompileTime]) =
        MathLib::KelvinVector::kelvinVectorToSymmetricTensor(sigma_avg);

    NumLib::interpolateToHigherOrderNodes<
        ShapeFunction, typename ShapeFunctionDisplacement::MeshElement,
        DisplacementDim>(element_, is_axially_symmetric_, p_L,
                         *process_data_.pressure_interpolated);
    NumLib::interpolateToHigherOrderNodes<
        ShapeFunction, typename ShapeFunctionDisplacement::MeshElement,
        DisplacementDim>(element_, is_axially_symmetric_, T,
                         *process_data_.temperature_interpolated);
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
unsigned ThermoRichardsMechanicsLocalAssembler<
    ShapeFunctionDisplacement, ShapeFunction,
    DisplacementDim>::getNumberOfIntegrationPoints() const
{
    return integration_method_.getNumberOfPoints();
}

template <typename ShapeFunctionDisplacement, typename ShapeFunction,
          int DisplacementDim>
typename MaterialLib::Solids::MechanicsBase<
    DisplacementDim>::MaterialStateVariables const&
ThermoRichardsMechanicsLocalAssembler<ShapeFunctionDisplacement, ShapeFunction,
                                      DisplacementDim>::
    getMaterialStateVariablesAt(unsigned integration_point) const
{
    return *material_states_[integration_point].material_state_variables;
}
}  // namespace ThermoRichardsMechanics
}  // namespace ProcessLib
