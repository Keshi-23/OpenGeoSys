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

#include "MathLib/TemplateWeightedPoint.h"

#include "MeshLib/Elements/FaceRule.h"
#include "MeshLib/Elements/Elements.h"

namespace MeshLib
{
/// \page BulkMappingDocuPage
/// [Documentation](https://www.opengeosys.org/pdf/BulkElementMappings.pdf) for
/// the mapping of a point given in local coordinates of a boundary face/element
/// to the corresponding bulk element point.

/// Maps the given lower dimensional boundary point \c wp of a line, i.e. the 1d
/// integration point given in local coordinates of a line, to higher
/// dimensional point of the triangle face (defined by the triangle element and
/// the face id) also in local coordinates of the triangle element.
/// \param tri the triangle element
/// \param face_id the id of the triangle face the point will be mapped on
/// \param wp the integration point of the lower dimensional element
/// \return the mapped point
MathLib::Point3d getBulkElementPoint(Tri const& tri,
                                     std::size_t const face_id,
                                     MathLib::WeightedPoint1D const& wp);

/// Maps the given lower dimensional boundary point \c wp of a line, i.e. the 1d
/// integration point given in local coordinates of a line, to higher dimensional
/// point of the quad face (defined by the quad element and the face id) also in
/// local coordinates of the quad face.
/// \param quad the quad element
/// \param face_id the id of the quad face the point will be mapped on
/// \param wp the integration point of the lower dimensional element
/// \return the mapped point
MathLib::Point3d getBulkElementPoint(Quad const& quad,
                                     std::size_t const face_id,
                                     MathLib::WeightedPoint1D const& wp);

/// Maps the given 2d boundary point \c wp of a quad face, given in local
/// coordinates of the quad, to 3d point existing on a hexahedron face also in
/// local coordinates.
MathLib::Point3d getBulkElementPoint(Hex const& hex,
                                     std::size_t const face_id,
                                     MathLib::WeightedPoint2D const& wp);

/// Maps the given lower dimensional 2d boundary point \c wp of a quad or
/// triangle element, given in local coordinates of the quad or triangle, to a
/// 3d point existing on a tet face also in local coordinates.
MathLib::Point3d getBulkElementPoint(Tet const& tet,
                                     std::size_t const face_id,
                                     MathLib::WeightedPoint2D const& wp);

/// Maps the given lower dimensional 2d boundary point \c wp of a quad or
/// triangle element, given in local coordinates of the quad or triangle, to a
/// 3d point existing on a prism face also in local coordinates.
MathLib::Point3d getBulkElementPoint(Prism const& prism,
                                     std::size_t const face_id,
                                     MathLib::WeightedPoint2D const& wp);

/// Maps the given lower dimensional 2d boundary point \c wp of a quad or
/// triangle element, given in local coordinates of the quad or triangle, to a
/// 3d point existing on a pyramid face also in local coordinates.
MathLib::Point3d getBulkElementPoint(Pyramid const& pyramid,
                                     std::size_t const face_id,
                                     MathLib::WeightedPoint2D const& wp);

/// Maps the given 1d boundary point \c wp of a boundary element, given in local
/// coordinates of the boundary element, to 3d point existing on a bulk element
/// also in local coordinates.
MathLib::Point3d getBulkElementPoint(Mesh const& mesh,
                                     std::size_t const bulk_element_id,
                                     std::size_t const bulk_face_id,
                                     MathLib::WeightedPoint1D const& wp);

/// Maps the given 2d boundary point \c wp of a boundary element, given in local
/// coordinates of the boundary element, to 3d point existing on a bulk element
/// also in local coordinates.
MathLib::Point3d getBulkElementPoint(Mesh const& mesh,
                                     std::size_t bulk_element_id,
                                     std::size_t bulk_face_id,
                                     MathLib::WeightedPoint2D const& wp);

// TODO disable the 3d elements in the local assembler creator
MathLib::Point3d getBulkElementPoint(Mesh const& mesh,
                                     std::size_t bulk_element_id,
                                     std::size_t bulk_face_id,
                                     MathLib::WeightedPoint3D const& wp);
}  // namespace MeshLib
