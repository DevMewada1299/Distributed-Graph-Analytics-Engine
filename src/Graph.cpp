#include "dgraph/Graph.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>

namespace dgraph {

Graph::Graph(MPI_Comm comm) : comm_(comm) {
    MPI_Comm_rank(comm_, &rank_);
    MPI_Comm_size(comm_, &size_);
}

Graph::~Graph() {}

void Graph::distributeVertices(VertexId total_vertices) {
    global_num_vertices_ = total_vertices;
    VertexId remainder = total_vertices % size_;
    VertexId chunk = total_vertices / size_;

    // Determine start and end for this rank
    if (static_cast<VertexId>(rank_) < remainder) {
        start_vertex_id_ = rank_ * (chunk + 1);
        end_vertex_id_ = start_vertex_id_ + chunk + 1;
    } else {
        start_vertex_id_ = rank_ * chunk + remainder;
        end_vertex_id_ = start_vertex_id_ + chunk;
    }
    local_num_vertices_ = end_vertex_id_ - start_vertex_id_;

    // Initialize row_ptr
    row_ptr_.assign(local_num_vertices_ + 1, 0);
}

void Graph::loadFromFile(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    VertexId num_v;
    if (rank_ == 0) {
        infile >> num_v;
    }
    
    // Broadcast number of vertices
    MPI_Bcast(&num_v, 1, MPI_UINT64_T, 0, comm_);
    distributeVertices(num_v);

    // Temporary storage for local adjacency list
    // Using vector of vectors to easily append edges
    std::vector<std::vector<VertexId>> local_adj(local_num_vertices_);
    
    // Re-open or reset file to read edges
    // In a real distributed system, we'd use MPI-IO or parallel file system strategies.
    // Here, for simplicity/portability, we accept the cost of re-reading or all-reading.
    // Optimization: Only Rank 0 reads and scatters? No, too much memory.
    // Optimization: All read, filter.
    
    // Reset to beginning (Rank 0 already read one line, others need to start)
    // Actually simpler if everyone opens their own stream or seeks.
    // Rank 0 is at line 2. Others are at line 1.
    // Let's close and reopen for everyone to be safe and simple.
    infile.close();
    infile.open(filename);
    
    VertexId dummy;
    infile >> dummy; // Skip num vertices

    VertexId src, dst;
    uint64_t edge_count = 0;
    
    while (infile >> src >> dst) {
        // If src belongs to this rank
        if (src >= start_vertex_id_ && src < end_vertex_id_) {
            VertexId local_src = src - start_vertex_id_;
            if (local_src < local_num_vertices_) {
                 local_adj[local_src].push_back(dst);
                 edge_count++;
            }
        }
    }
    infile.close();

    // Convert to CSR
    // row_ptr_ is already sized
    row_ptr_[0] = 0;
    col_ind_.reserve(edge_count);
    // weights_.reserve(edge_count); // Assuming unweighted for now or default weight 1.0

    for (size_t i = 0; i < local_num_vertices_; ++i) {
        // Sort neighbors for better cache locality / intersection perf
        std::sort(local_adj[i].begin(), local_adj[i].end());
        
        for (const auto& neighbor : local_adj[i]) {
            col_ind_.push_back(neighbor);
            weights_.push_back(1.0f); // Default weight
        }
        row_ptr_[i + 1] = col_ind_.size();
    }
    
    if (rank_ == 0) {
        std::cout << "Graph loaded. Global Vertices: " << global_num_vertices_ 
                  << ". Local edges on Rank 0: " << edge_count << std::endl;
    }
}

} // namespace dgraph
