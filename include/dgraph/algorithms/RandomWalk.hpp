#pragma once

#include "../Graph.hpp"
#include "../Engine.hpp"
#include <vector>
#include <random>
#include <fstream>
#include <sstream>

namespace dgraph {

struct Walk {
    uint64_t id;      // Unique walk ID
    VertexId start_node;
    std::vector<VertexId> path;
};

// Wrapper for Message Accumulation
// We receive multiple walks. Engine's reduce needs to merge them.
// Actually Engine's reduce assumes ONE value per vertex.
// But here, a vertex might receive multiple walkers from different neighbors.
// So AccT should be std::vector<Walk>.
struct WalkList {
    std::vector<Walk> walks;
};

class RandomWalk {
public:
    RandomWalk(Graph& graph) : graph_(graph), engine_(graph) {}

    // walk_length: Length of walk
    // num_walks: Walks per node
    void compute(int walk_length, int num_walks, const std::string& output_prefix) {
        VertexId num_local = graph_.numLocalVertices();
        VertexId start_id = graph_.globalStartId();
        
        // Active walks residing at local vertices
        // Map: Local Vertex Index -> List of walks currently parked there
        std::vector<std::vector<Walk>> active_walks(num_local);
        
        // Initialize walks
        for(VertexId i=0; i<num_local; ++i) {
            for(int w=0; w<num_walks; ++w) {
                Walk walk;
                walk.id = (uint64_t(start_id + i) << 32) | w; // Simple unique ID
                walk.start_node = start_id + i;
                walk.path.push_back(start_id + i);
                active_walks[i].push_back(walk);
            }
        }

        // Random Number Generator
        std::mt19937 rng(1234 + graph_.getRank()); 

        // Steps
        for(int step = 0; step < walk_length; ++step) {
            
            auto scatter = [&](VertexId local_id, std::vector<std::vector<Message<Walk>>>& buffers) {
                // For each walk currently at this node, move it to a neighbor
                auto& walks = active_walks[local_id];
                if (walks.empty()) return;
                
                auto neighbors = graph_.getNeighbors(local_id);
                VertexId degree = graph_.getOutDegree(local_id);
                
                if (degree == 0) {
                    // Dead end: Stay here or terminate? 
                    // Let's just keep them here (they won't move). 
                    // In BSP scatter, if we don't send, they disappear from "next state" logic if we clear active_walks.
                    // So we must handle them.
                    // Actually, if degree 0, we can't send. They just stay.
                    // But 'active_walks' will be overwritten by 'apply'. 
                    // So we need to send to SELF? 
                    // Engine doesn't support self-messages easily without going through network? 
                    // Actually Engine handles it if getOwner(self) is me.
                    for (auto& w : walks) {
                        buffers[engine_.getRank()].push_back({start_id + local_id, w});
                    }
                    return;
                }

                std::uniform_int_distribution<VertexId> dist(0, degree - 1);
                
                for (auto& w : walks) {
                    // Pick random neighbor
                    VertexId offset = dist(rng);
                    VertexId next_hop = *(neighbors.first + offset);
                    
                    // Update walk
                    w.path.push_back(next_hop);
                    
                    int owner = engine_.getOwner(next_hop);
                    buffers[owner].push_back({next_hop, w});
                }
            };

            auto reduce = [&](WalkList& acc, const Walk& val) {
                acc.walks.push_back(val);
            };

            // Workaround: We can swap to a 'next_active_walks' buffer.
            
            std::vector<std::vector<Walk>> next_active_walks(num_local);
            
            // Re-bind apply to use next_active_walks
            auto apply_safe = [&](VertexId global_dst, const WalkList& val) {
                VertexId local_idx = global_dst - start_id;
                if (local_idx < num_local) {
                    next_active_walks[local_idx] = val.walks;
                }
            };

            engine_.run(1, scatter, reduce, apply_safe);
            
            active_walks = std::move(next_active_walks);
        }

        // Output to file
        std::stringstream ss;
        ss << output_prefix << "_" << graph_.getRank() << ".txt";
        std::ofstream outfile(ss.str());
        
        for (const auto& list : active_walks) {
            for (const auto& w : list) {
                for (size_t i = 0; i < w.path.size(); ++i) {
                    outfile << w.path[i] << (i == w.path.size() - 1 ? "" : " ");
                }
                outfile << "\n";
            }
        }
        outfile.close();
    }

private:
    Graph& graph_;
    Engine<Walk, WalkList> engine_;
};

} // namespace dgraph
