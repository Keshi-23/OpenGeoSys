/**
 * \file
 * \copyright
 * Copyright (c) 2012-2021, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 * Created on October 1, 2021, 2:13 PM
 */

#include "NodePartitionedMesh.h"

namespace MeshLib
{
std::vector<int> getEndNodeIDRanks(
    std::vector<std::size_t> const& n_active_nodes_at_rank)
{
    std::vector<int> data;
    int id_of_end_node_of_partition = 0;
    std::transform(
        n_active_nodes_at_rank.begin(), n_active_nodes_at_rank.end(),
        std::back_inserter(data),
        [&id_of_end_node_of_partition](std::size_t const n_active_node)
        {
            id_of_end_node_of_partition += n_active_node;
            return id_of_end_node_of_partition;
        });

    return data;
}

NodePartitionedMesh::NodePartitionedMesh(
    const std::string& name,
    const std::vector<Node*>& nodes,
    const std::vector<std::size_t>& glb_node_ids,
    const std::vector<Element*>& elements,
    Properties properties,
    const std::size_t n_global_base_nodes,
    const std::size_t n_global_nodes,
    const std::size_t n_active_base_nodes,
    const std::size_t n_active_nodes,
    std::vector<std::size_t>&& n_active_base_nodes_at_rank,
    std::vector<std::size_t>&& n_active_nodes_at_rank)
    : Mesh(name, nodes, elements, properties),
      _global_node_ids(glb_node_ids),
      _n_global_base_nodes(n_global_base_nodes),
      _n_global_nodes(n_global_nodes),
      _n_active_base_nodes(n_active_base_nodes),
      _n_active_nodes(n_active_nodes),
      _n_active_base_nodes_at_rank(std::move(n_active_base_nodes_at_rank)),
      _n_active_nodes_at_rank(std::move(n_active_nodes_at_rank)),
      _end_node_id_ranks(getEndNodeIDRanks(_n_active_nodes_at_rank)),
      _is_single_thread(false)
{
}

bool NodePartitionedMesh::isGhostNode(const std::size_t node_id) const
{
    if (node_id < _n_active_base_nodes)
    {
        return false;
    }
    if (!isBaseNode(*_nodes[node_id], getElementsConnectedToNode(node_id)) &&
        node_id < getLargestActiveNodeID())
    {
        return false;
    }
    return true;
}

std::size_t NodePartitionedMesh::getMaximumNConnectedNodesToNode() const
{
    auto const& nodes_connections =
        MeshLib::calculateNodesConnectedByElements(*this);
    auto const max_connections = std::max_element(
        nodes_connections.cbegin(), nodes_connections.cend(),
        [](auto const& connections_node_a, auto const& connections_node_b)
        { return (connections_node_a.size() < connections_node_b.size()); });
    // Return the number of connected nodes +1 for the node itself.
    return static_cast<std::size_t>(max_connections->size() + 1);
}

int NodePartitionedMesh::getPartitionID(const int global_node_id) const
{
    return std::upper_bound(std::cbegin(_end_node_id_ranks),
                            std::cend(_end_node_id_ranks), global_node_id) -
           _end_node_id_ranks.begin();
}
}  // namespace MeshLib