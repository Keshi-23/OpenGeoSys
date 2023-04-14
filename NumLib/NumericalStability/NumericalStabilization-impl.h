/**
 * \file
 * \copyright
 * Copyright (c) 2012-2023, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 * Created on June 24, 2022, 12:53 PM
 */

#pragma once

#include <limits>
#include <typeinfo>

namespace NumLib
{
template <typename IPData, typename FluxVectorType, typename Derived>
void assembleOriginalAdvectionMatrix(
    IPData const& ip_data_vector,
    std::vector<FluxVectorType> const& ip_flux_vector,
    Eigen::MatrixBase<Derived>& laplacian_matrix)
{
    for (std::size_t ip = 0; ip < ip_flux_vector.size(); ++ip)
    {
        auto const& ip_data = ip_data_vector[ip];
        auto const w = ip_data.integration_weight;
        auto const& N = ip_data.N;
        auto const& dNdx = ip_data.dNdx;
        laplacian_matrix.noalias() +=
            N.transpose() * ip_flux_vector[ip].transpose() * dNdx * w;
    }
}

template <typename Derived>
void applyFullUpwind(Eigen::VectorXd const& quasi_nodal_flux,
                     Eigen::MatrixBase<Derived>& laplacian_matrix)
{
    Eigen::VectorXd const down_mask =
        (quasi_nodal_flux.array() < 0).cast<double>();
    Eigen::VectorXd const down = quasi_nodal_flux.cwiseProduct(down_mask);

    double const q_in = -down.sum();
    if (q_in < std::numeric_limits<double>::epsilon())
    {
        return;
    }

    Eigen::VectorXd const up_mask =
        (quasi_nodal_flux.array() >= 0).cast<double>();
    Eigen::VectorXd const up = quasi_nodal_flux.cwiseProduct(up_mask);

    laplacian_matrix.diagonal().noalias() += up;
    laplacian_matrix.noalias() += down * up.transpose() / q_in;
}

template <typename IPData, typename FluxVectorType, typename Derived>
void applyFullUpwind(IPData const& ip_data_vector,
                     std::vector<FluxVectorType> const& ip_flux_vector,
                     Eigen::MatrixBase<Derived>& laplacian_matrix)
{
    Eigen::VectorXd quasi_nodal_flux =
        Eigen::VectorXd::Zero(laplacian_matrix.rows());

    for (std::size_t ip = 0; ip < ip_flux_vector.size(); ++ip)
    {
        auto const& ip_data = ip_data_vector[ip];
        auto const w = ip_data.integration_weight;
        auto const& dNdx = ip_data.dNdx;

        quasi_nodal_flux.noalias() -= ip_flux_vector[ip].transpose() * dNdx * w;
    }

    applyFullUpwind(quasi_nodal_flux, laplacian_matrix);
}

template <typename IPData, typename FluxVectorType, typename Derived>
void assembleAdvectionMatrix(NumericalStabilization const* const stabilizer,
                             IPData const& ip_data_vector,
                             double const average_velocity,
                             std::vector<FluxVectorType> const& ip_flux_vector,
                             Eigen::MatrixBase<Derived>& laplacian_matrix)
{
    if (stabilizer)
    {
        auto const& stabilizer_ref = *(stabilizer);
        if (typeid(stabilizer_ref) == typeid(NumLib::FullUpwind) &&
            (average_velocity > stabilizer->getCutoffVelocity()))
        {
            applyFullUpwind(ip_data_vector, ip_flux_vector, laplacian_matrix);
            return;
        }
    }

    assembleOriginalAdvectionMatrix(ip_data_vector, ip_flux_vector,
                                    laplacian_matrix);
}

}  // namespace NumLib
