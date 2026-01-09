#pragma once

#include "../Graph.hpp"
#include "../Engine.hpp"
#include <vector>
#include <limits>
#include <algorithm>

namespace dgraph {

class BFS {
public:
    BFS(Graph& graph) : graph_(graph), engine_(graph) {}

    std::vector<uint64_t> compute(VertexId source_node, int max_iterations = 100) {
        VertexId num_local = graph_.numLocalVertices();
        VertexId start_id = graph_.globalStartId();
        
        const uint64_t INF = std::numeric_limits<uint64_t>::max();
        std::vector<uint64_t> dist(num_local, INF);
        
        // Initialize source
        VertexId local_source = -1;
        if (source_node >= start_id && source_node < start_id + num_local) {
            local_source = source_node - start_id;
            dist[local_source] = 0;
        }

        // Boolean to track convergence
        bool changed = true;
        int iter = 0;
        
        while (changed && iter < max_iterations) {
            changed = false;
            int local_changed = 0;

            // Scatter: If I have a valid distance, send (dist + 1) to neighbors
            // Optimization: Only send if I just updated my distance? 
            // For simple BSP, we can just send if dist != INF.
            // Better: active list. But Engine scans all.
            // We can check if dist == iter. (Level-sync).
            // Yes, BFS only propagates if dist == iter.
            
            auto scatter = [&](VertexId local_id, std::vector<std::vector<Message<uint64_t>>>& buffers) {
                if (dist[local_id] == (uint64_t)iter) { // Only active frontier expands
                    uint64_t new_dist = dist[local_id] + 1;
                    auto neighbors = graph_.getNeighbors(local_id);
                    for (const VertexId* it = neighbors.first; it != neighbors.second; ++it) {
                        VertexId global_dst = *it;
                        int owner = engine_.getOwner(global_dst);
                        buffers[owner].push_back({global_dst, new_dist});
                    }
                }
            };

            auto reduce = [&](DistWrapper& acc, const uint64_t& val) {
                if (val < acc.d) acc.d = val;
            };

            auto apply = [&](VertexId global_dst, const DistWrapper& val) {
                VertexId local_idx = global_dst - start_id;
                if (local_idx < num_local) {
                    if (val.d < dist[local_idx]) {
                        dist[local_idx] = val.d;
                        local_changed = 1;
                    }
                }
            };

            // Run 1 iteration
            // Note: reduce init value should be INF
            // But Engine default constructs AccT. uint64_t default is 0? No, undefined or 0.
            // We need to handle this. Engine code: "accumulator = AccT(); // Reset".
            // If AccT is uint64_t, it's 0.
            // This is a problem for min-reduction. 
            // Fix: Change MsgT/AccT to a struct with constructor, or handle 0 in reduce?
            // Or change Engine to allow init value.
            // Workaround: Use a wrapper struct for BFS Message.
            
            engine_.run(1, scatter, reduce, apply);

            // Check global convergence
            int global_changed = 0;
            MPI_Allreduce(&local_changed, &global_changed, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
            changed = (global_changed > 0);
            
            iter++;
        }

        return dist;
    }

private:
    Graph& graph_;
    // MsgT = uint64_t (distance). AccT needs to default to INF?
    // Since Engine default constructs AccT, we need a wrapper.
    struct DistWrapper {
        uint64_t d;
        DistWrapper() : d(std::numeric_limits<uint64_t>::max()) {}
        DistWrapper(uint64_t val) : d(val) {}
        bool operator<(const DistWrapper& other) const { return d < other.d; }
    };
    
    Engine<uint64_t, DistWrapper> engine_;
};

} // namespace dgraph
