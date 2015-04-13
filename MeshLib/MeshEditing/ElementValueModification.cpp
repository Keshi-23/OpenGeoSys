/**
 * \file
 * \author Karsten Rink
 * \date   2013-04-04
 * \brief  Implementation of the ElementValueModification class.
 *
 * \copyright
 * Copyright (c) 2012-2015, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include "ElementValueModification.h"

#include <algorithm>

#include "logog/include/logog.hpp"

#include "MeshLib/Mesh.h"
#include "MeshLib/Elements/Element.h"


namespace MeshLib {

std::vector<unsigned> ElementValueModification::getMeshValues(const MeshLib::Mesh &mesh)
{
	const std::size_t nElements (mesh.getNElements());
	std::vector<unsigned> value_mapping;
	for (unsigned i=0; i<nElements; ++i)
	{
		bool exists(false);
		unsigned value (mesh.getElement(i)->getValue());
		const unsigned nValues (value_mapping.size());
		for (unsigned j=0; j<nValues; ++j)
		{
			if (value == value_mapping[j])
			{
				exists = true;
				break;
			}
		}
		if (!exists)
			value_mapping.push_back(value);
	}

	std::sort(value_mapping.begin(), value_mapping.end());
	return value_mapping;
}

bool ElementValueModification::replace(MeshLib::Mesh &mesh,
	std::string const& property_name, unsigned old_value, unsigned new_value,
	bool replace_if_exists)
{
	boost::optional<MeshLib::PropertyVector<int> &> optional_property_value_vec(
		mesh.getProperties().getPropertyVector<int>(property_name)
	);

	if (!optional_property_value_vec) {
		return false;
	}

	MeshLib::PropertyVector<int> & property_value_vec(
		optional_property_value_vec.get()
	);
	const std::size_t n_property_values(property_value_vec.size());

	if (!replace_if_exists) {
		for (std::size_t i=0; i<n_property_values; ++i) {
			if (property_value_vec[i] == new_value) {
				WARN ("ElementValueModification::replaceElementValue() "
					"- Replacement value \"%d\" is already taken, "
					"no changes have been made.", new_value);
				return false;
			}
		}
	}

	for (std::size_t i=0; i<n_property_values; ++i) {
		if (property_value_vec[i] == old_value)
			property_value_vec[i] = new_value;
	}

	return true;
}

bool ElementValueModification::replace(MeshLib::Mesh &mesh,
	unsigned old_value, unsigned new_value, bool replace_if_exists)
{
	return replace(mesh, "MaterialIDs", old_value, new_value, replace_if_exists);
}

unsigned ElementValueModification::condense(MeshLib::Mesh &mesh)
{
	boost::optional<MeshLib::PropertyVector<int> &>
		optional_property_value_vec(
			mesh.getProperties().getPropertyVector<int>("MaterialIDs")
		);

	if (!optional_property_value_vec) {
		return 0;
	}

	MeshLib::PropertyVector<int> & property_value_vector(
		optional_property_value_vec.get()
	);
	std::vector<int> value_mapping(
		getSortedPropertyValues(property_value_vector)
	);

	std::vector<int> reverse_mapping(value_mapping.back()+1, 0);
	const unsigned nValues (value_mapping.size());
	for (unsigned i=0; i<nValues; ++i)
		reverse_mapping[value_mapping[i]] = i;

	std::size_t const n_property_values(property_value_vector.size());
	for (std::size_t i=0; i<n_property_values; ++i)
		property_value_vector[i] = reverse_mapping[property_value_vector[i]];

	return nValues;
}

unsigned ElementValueModification::setByElementType(MeshLib::Mesh &mesh, MeshElemType ele_type, unsigned new_value)
{
	std::vector<MeshLib::Element*> &elements (const_cast<std::vector<MeshLib::Element*>&>(mesh.getElements()));
	unsigned nValues = 0;
	for (auto e : elements) {
		if (e->getGeomType()!=ele_type)
			continue;
		e->setValue(new_value);
		nValues++;
	}

	return nValues;
}

} // end namespace MeshLib
