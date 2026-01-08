#pragma once

#include "Types.hpp"
#include <vector>
#include <string>
#include "MPI_Wrapper.hpp"

#ifdef _OPENMP
#include <omp.h>
#else
inline int omp_get_thread_num() { return 0; }
inline int omp_get_num_threads() { return 1; }
#endif

namespace dgraph {

class Graph {
public:
    Graph(MPI_Comm comm);
    ~Graph();

    // Load graph from an edge list file (simplified for now: every rank reads and filters)
    // In production, parallel I/O should be used.
    void loadFromFile(const std::string& filename);

    // Getters
    VertexId numLocalVertices() const { return local_num_vertices_; }
    VertexId numGlobalVertices() const { return global_num_vertices_; }
    uint64_t numLocalEdges() const { return row_ptr_.back(); }
    
    VertexId globalStartId() const { return start_vertex_id_; }
    VertexId globalEndId() const { return end_vertex_id_; } // Exclusive

    // CSR Access
    const std::vector<uint64_t>& getRowPtr() const { return row_ptr_; }
    const std::vector<VertexId>& getColInd() const { return col_ind_; }
    const std::vector<EdgeWeight>& getWeights() const { return weights_; }
    
    // Out-degree of a local vertex (by local index 0 to numLocalVertices-1)
    VertexId getOutDegree(VertexId local_id) const {
        return row_ptr_[local_id + 1] - row_ptr_[local_id];
    }
    
    // Get neighbors of a local vertex
    // Returns pair of start pointer and end pointer in col_ind
    std::pair<const VertexId*, const VertexId*> getNeighbors(VertexId local_id) const {
        uint64_t start = row_ptr_[local_id];
        uint64_t end = row_ptr_[local_id + 1];
        return {&col_ind_[start], &col_ind_[end]};
    }

private:
    MPI_Comm comm_;
    int rank_;
    int size_;

    VertexId global_num_vertices_ = 0;
    VertexId local_num_vertices_ = 0;
    
    // Range of vertices owned by this rank: [start_vertex_id_, end_vertex_id_)
    VertexId start_vertex_id_ = 0;
    VertexId end_vertex_id_ = 0;

    // CSR storage for local vertices' outgoing edges
    // row_ptr has size local_num_vertices_ + 1
    std::vector<uint64_t> row_ptr_;
    std::vector<VertexId> col_ind_;
    std::vector<EdgeWeight> weights_;

    void distributeVertices(VertexId total_vertices);
};

} // namespace dgraph
