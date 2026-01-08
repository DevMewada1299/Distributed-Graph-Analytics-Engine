#pragma once

#include "../Graph.hpp"
#include "../Engine.hpp"
#include <vector>
#include <iostream>

namespace dgraph {

class PageRank {
public:
    PageRank(Graph& graph) : graph_(graph), engine_(graph) {}

    std::vector<double> compute(int iterations = 10, double damping = 0.85) {
        VertexId num_local = graph_.numLocalVertices();
        VertexId num_global = graph_.numGlobalVertices();
        
        std::vector<double> pr_values(num_local, 1.0);
        
        for (int iter = 0; iter < iterations; ++iter) {
            double local_dangling_sum = 0.0;
            
            #pragma omp parallel for reduction(+:local_dangling_sum)
            for (VertexId i = 0; i < num_local; ++i) {
                if (graph_.getOutDegree(i) == 0) {
                    local_dangling_sum += pr_values[i];
                }
            }

            double global_dangling_sum = 0.0;
            MPI_Allreduce(&local_dangling_sum, &global_dangling_sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
            
            auto scatter = [&](VertexId local_id, std::vector<std::vector<Message<double>>>& buffers) {
                VertexId degree = graph_.getOutDegree(local_id);
                if (degree > 0) {
                    double contribution = pr_values[local_id] / degree;
                    auto neighbors = graph_.getNeighbors(local_id);
                    
                    for (const VertexId* it = neighbors.first; it != neighbors.second; ++it) {
                        VertexId global_dst = *it;
                        int owner = engine_.getOwner(global_dst);
                        buffers[owner].push_back({global_dst, contribution});
                    }
                }
            };

            auto reduce = [&](double& acc, const double& val) {
                acc += val;
            };

            std::vector<double> next_pr(num_local);
            double base_value = (1.0 - damping) + (damping * global_dangling_sum / num_global);
            
            #pragma omp parallel for
            for(VertexId i=0; i<num_local; ++i) next_pr[i] = base_value;

            auto apply = [&](VertexId global_dst, const double& sum) {
                VertexId local_idx = global_dst - graph_.globalStartId();
                if (local_idx < num_local) {
                    next_pr[local_idx] += damping * sum;
                }
            };

            engine_.run(1, scatter, reduce, apply);
            
            pr_values = std::move(next_pr);
            
            int rank;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            if (rank == 0) std::cout << "Iteration " << iter + 1 << " complete." << std::endl;
        }
        
        return pr_values;
    }

private:
    Graph& graph_;
    Engine<double, double> engine_;
};

} // namespace dgraph
