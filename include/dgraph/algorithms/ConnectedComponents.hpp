#pragma once

#include "../Graph.hpp"
#include "../Engine.hpp"
#include <vector>
#include <algorithm>
#include <limits>

namespace dgraph {

class ConnectedComponents {
public:
    ConnectedComponents(Graph& graph) : graph_(graph), engine_(graph) {}

    std::vector<VertexId> compute(int max_iterations = 100) {
        VertexId num_local = graph_.numLocalVertices();
        VertexId start_id = graph_.globalStartId();
        
        // Init component ID to self
        std::vector<VertexId> cc(num_local);
        for(VertexId i=0; i<num_local; ++i) {
            cc[i] = start_id + i;
        }

        bool changed = true;
        int iter = 0;
        
        while (changed && iter < max_iterations) {
            changed = false;
            int local_changed = 0;

            auto scatter = [&](VertexId local_id, std::vector<std::vector<Message<VertexId>>>& buffers) {
                // Optimization: In standard CC, we always send our Component ID to neighbors.
                // Or better: Only send if my Component ID changed recently?
                // For simplicity, always send current CC ID.
                // Optimization 2: Min-label propagation.
                
                VertexId current_cc = cc[local_id];
                // Only send if I am smaller than neighbors? We don't know neighbors' values.
                // Just broadcast current_cc.
                
                auto neighbors = graph_.getNeighbors(local_id);
                for (const VertexId* it = neighbors.first; it != neighbors.second; ++it) {
                    VertexId global_dst = *it;
                    int owner = engine_.getOwner(global_dst);
                    // Optimization: Don't send if dst < current_cc (dst is definitely smaller/equal if it's its own ID).
                    // But we can't know dst's current state.
                    
                    buffers[owner].push_back({global_dst, current_cc});
                }
            };

            auto reduce = [&](MinIdWrapper& acc, const VertexId& val) {
                if (val < acc.id) acc.id = val;
            };

            auto apply = [&](VertexId global_dst, const MinIdWrapper& val) {
                VertexId local_idx = global_dst - start_id;
                if (local_idx < num_local) {
                    if (val.id < cc[local_idx]) {
                        cc[local_idx] = val.id;
                        local_changed = 1;
                    }
                }
            };

            engine_.run(1, scatter, reduce, apply);

            int global_changed = 0;
            MPI_Allreduce(&local_changed, &global_changed, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
            changed = (global_changed > 0);
            iter++;
        }

        return cc;
    }

private:
    Graph& graph_;
    
    struct MinIdWrapper {
        VertexId id;
        MinIdWrapper() : id(std::numeric_limits<VertexId>::max()) {}
        MinIdWrapper(VertexId val) : id(val) {}
    };

    Engine<VertexId, MinIdWrapper> engine_;
};

} // namespace dgraph
