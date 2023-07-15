/**
 * \file
 * \copyright
 * Copyright (c) 2012-2023, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#pragma once

#include "EdgeReturn.h"
#include "Element.h"
#include "PyramidRule.h"

namespace MeshLib
{

/**
 * This class represents a 3d pyramid element with 5 nodes.
 * The following sketch shows the node and edge numbering.
 * \anchor Pyramid5NodeAndEdgeNumbering
 * \code
 *
 *               4
 *             //|\
 *            // | \
 *          7//  |  \6
 *          //   |5  \
 *         //    |    \
 *        3/.... |.....2
 *       ./      |  2 /
 *      ./4      |   /
 *    3./        |  /1
 *    ./         | /
 *   ./          |/
 *  0------------1
 *        0
 * \endcode

 */
class PyramidRule5 : public PyramidRule
{
public:
    /// Constant: The number of all nodes for this element
    static const unsigned n_all_nodes = 5u;

    /// Constant: The FEM type of the element
    static const CellType cell_type = CellType::PYRAMID5;

    /// Constant: Local node index table for faces
    static const unsigned face_nodes[5][4];

    /// Constant: Local node index table for edge
    static const unsigned edge_nodes[8][2];

    /// Constant: Table for the number of nodes for each face
    static const unsigned n_face_nodes[5];

    /// Returns the i-th edge of the element.
    using EdgeReturn = MeshLib::LinearEdgeReturn;

    /// Returns the i-th face of the element.
    static const Element* getFace(const Element* e, unsigned i);

    /// Returns the ID of a face given an array of nodes.
    static unsigned identifyFace(Node const* const* _nodes,
                                 Node const* nodes[3])
    {
        return CellRule::identifyFace<PyramidRule5>(_nodes, nodes);
    }
}; /* class */

}  // namespace MeshLib
