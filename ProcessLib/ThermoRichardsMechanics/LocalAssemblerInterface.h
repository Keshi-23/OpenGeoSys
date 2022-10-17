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
#include "MaterialLib/SolidModels/SelectSolidConstitutiveRelation.h"
#include "NumLib/Extrapolation/ExtrapolatableElement.h"
#include "NumLib/Fem/Integration/GenericIntegrationMethod.h"
#include "ProcessLib/LocalAssemblerInterface.h"
#include "ProcessLib/ThermoRichardsMechanics/ConstitutiveSetting.h"
#include "ProcessLib/ThermoRichardsMechanics/ThermoRichardsMechanicsProcessData.h"
#include "ProcessLib/Utils/SetOrGetIntegrationPointData.h"

namespace ProcessLib::ThermoRichardsMechanics
{
template <int DisplacementDim>
struct LocalAssemblerInterface : public ProcessLib::LocalAssemblerInterface,
                                 public NumLib::ExtrapolatableElement
{
    LocalAssemblerInterface(
        MeshLib::Element const& e,
        NumLib::GenericIntegrationMethod const& integration_method,
        bool const is_axially_symmetric,
        ThermoRichardsMechanicsProcessData<DisplacementDim>& process_data)
        : process_data_(process_data),
          integration_method_(integration_method),
          element_(e),
          is_axially_symmetric_(is_axially_symmetric),
          solid_material_(MaterialLib::Solids::selectSolidConstitutiveRelation(
              process_data_.solid_materials, process_data_.material_ids,
              e.getID()))
    {
        unsigned const n_integration_points =
            integration_method_.getNumberOfPoints();

        current_states_.resize(n_integration_points);
        prev_states_.resize(n_integration_points);
        output_data_.resize(n_integration_points);

        material_states_.reserve(n_integration_points);
        for (unsigned ip = 0; ip < n_integration_points; ++ip)
        {
            material_states_.emplace_back(solid_material_);
        }
    }

    std::size_t setIPDataInitialConditions(std::string const& name,
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
            return ProcessLib::setIntegrationPointKelvinVectorData<
                DisplacementDim>(
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
                values, current_states_, [](auto& state) -> auto& {
                    return state.transport_poro_data.phi;
                });
        }
        if (name == "swelling_stress_ip")
        {
            return ProcessLib::setIntegrationPointKelvinVectorData<
                DisplacementDim>(
                values, current_states_,
                [](auto& cs) -> auto& { return cs.swelling_data.sigma_sw; });
        }
        if (name == "epsilon_ip")
        {
            return ProcessLib::setIntegrationPointKelvinVectorData<
                DisplacementDim>(
                values, current_states_,
                [](auto& cs) -> auto& { return cs.eps_data.eps; });
        }
        return 0;
    }

    // TODO move to NumLib::ExtrapolatableElement
    unsigned getNumberOfIntegrationPoints() const
    {
        return integration_method_.getNumberOfPoints();
    }

    typename MaterialLib::Solids::MechanicsBase<
        DisplacementDim>::MaterialStateVariables const&
    getMaterialStateVariablesAt(unsigned integration_point) const
    {
        return *material_states_[integration_point].material_state_variables;
    }

    void postTimestepConcrete(Eigen::VectorXd const& /*local_x*/,
                              double const /*t*/,
                              double const /*dt*/) override
    {
        unsigned const n_integration_points =
            integration_method_.getNumberOfPoints();

        for (unsigned ip = 0; ip < n_integration_points; ip++)
        {
            // TODO re-evaluate part of the assembly in order to be consistent?
            material_states_[ip].pushBackState();
        }

        prev_states_ = current_states_;
    }

    static auto getReflectionDataForOutput()
    {
        using Self = LocalAssemblerInterface<DisplacementDim>;

        return ProcessLib::Reflection::reflectWithoutName(
            &Self::current_states_, &Self::output_data_);
    }

protected:
    ThermoRichardsMechanicsProcessData<DisplacementDim>& process_data_;

    std::vector<StatefulData<DisplacementDim>>
        current_states_;  // TODO maybe do not store but rather re-evaluate for
                          // state update
    std::vector<StatefulData<DisplacementDim>> prev_states_;

    // Material state is special, because it contains both the current and the
    // old state.
    std::vector<MaterialStateData<DisplacementDim>> material_states_;

    NumLib::GenericIntegrationMethod const& integration_method_;
    MeshLib::Element const& element_;
    bool const is_axially_symmetric_;

    MaterialLib::Solids::MechanicsBase<DisplacementDim> const& solid_material_;

    std::vector<OutputData<DisplacementDim>> output_data_;
};

}  // namespace ProcessLib::ThermoRichardsMechanics
