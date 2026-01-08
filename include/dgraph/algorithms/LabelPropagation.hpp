#pragma once

#include "../Graph.hpp"
#include "../Engine.hpp"
#include <vector>
#include <map>
#include <random>
#include <algorithm>

namespace dgraph {

class LabelPropagation {
public:
    LabelPropagation(Graph& graph) : graph_(graph), engine_(graph) {}

    std::vector<VertexId> compute(int iterations = 10) {
        VertexId num_local = graph_.numLocalVertices();
        VertexId start_id = graph_.globalStartId();
        
        // Initialize labels: each vertex is its own community
        std::vector<VertexId> labels(num_local);
        #pragma omp parallel for
        for (VertexId i = 0; i < num_local; ++i) {
            labels[i] = start_id + i;
        }

        // Engine Accumulator Type: Map of Label -> Count
        using AccT = std::map<VertexId, int>;

        for (int iter = 0; iter < iterations; ++iter) {
            
            auto scatter = [&](VertexId local_id, std::vector<std::vector<Message<VertexId>>>& buffers) {
                VertexId my_label = labels[local_id];
                auto neighbors = graph_.getNeighbors(local_id);
                
                for (const VertexId* it = neighbors.first; it != neighbors.second; ++it) {
                    VertexId global_dst = *it;
                    int owner = engine_.getOwner(global_dst);
                    buffers[owner].push_back({global_dst, my_label});
                }
            };

            auto reduce = [&](AccT& acc, const VertexId& label) {
                acc[label]++;
            };

            // Temporary storage for synchronous update
            // We only update vertices that received messages (neighbors).
            // But if a vertex has no neighbors, it keeps its label.
            // Engine apply is only called for vertices with incoming messages.
            std::vector<VertexId> next_labels = labels;

            auto apply = [&](VertexId global_dst, const AccT& acc) {
                VertexId local_idx = global_dst - graph_.globalStartId();
                if (local_idx < num_local) {
                    // Find label with max count
                    VertexId best_label = labels[local_idx]; // Default to current
                    int max_count = -1;
                    
                    // Tie-breaking: Use smallest label ID for determinism, or random.
                    // Map iterates in sorted order of Key (Label), so simple traversal works for deterministic lowest ID.
                    
                    for (const auto& pair : acc) {
                        if (pair.second > max_count) {
                            max_count = pair.second;
                            best_label = pair.first;
                        } else if (pair.second == max_count) {
                            // Tie: stick with current if it's one of them?
                            // Or strictly lowest?
                            // Standard LPA often says random. 
                            // For this implementation, lowest label ID (map order) wins on tie if we use >.
                            // But we initialized best_label to current.
                            // Let's just pick the max count one.
                        }
                    }
                    next_labels[local_idx] = best_label;
                }
            };

            engine_.run(1, scatter, reduce, apply);
            
            labels = std::move(next_labels);
            
            int rank;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            if (rank == 0) std::cout << "LPA Iteration " << iter + 1 << " complete." << std::endl;
        }
        
        return labels;
    }

private:
    Graph& graph_;
    Engine<VertexId, std::map<VertexId, int>> engine_;
};

} // namespace dgraph
