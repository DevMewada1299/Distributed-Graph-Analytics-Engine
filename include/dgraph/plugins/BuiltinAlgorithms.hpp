#pragma once

#include "../IAlgorithm.hpp"
#include "../algorithms/BFS.hpp"
#include "../algorithms/ConnectedComponents.hpp"
#include "../algorithms/PageRank.hpp"
#include "../algorithms/LabelPropagation.hpp"
#include "../algorithms/RandomWalk.hpp"
#include <iostream>
#include <iomanip>

namespace dgraph {

class BFSPlugin : public IAlgorithm {
public:
    std::string name() const override { return "bfs"; }
    void run(Graph& graph, const std::vector<std::string>& args) override {
        VertexId source = 0;
        if (!args.empty()) source = std::stoull(args[0]);
        
        int rank = graph.getRank();
        if (rank == 0) std::cout << "Running BFS from source " << source << "..." << std::endl;
        
        BFS bfs(graph);
        auto results = bfs.compute(source);
        
        // Output logic (Distributed print)
        for (int r = 0; r < graph.getSize(); ++r) {
            if (rank == r) {
                for (VertexId i = 0; i < graph.numLocalVertices(); ++i) {
                    VertexId global_id = graph.globalStartId() + i;
                    std::cout << "V[" << global_id << "]: BFS_Dist=";
                    if (results[i] == std::numeric_limits<uint64_t>::max()) 
                        std::cout << "INF";
                    else 
                        std::cout << results[i];
                    std::cout << std::endl;
                }
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
};
REGISTER_ALGORITHM(BFSPlugin);

class CCPlugin : public IAlgorithm {
public:
    std::string name() const override { return "cc"; }
    void run(Graph& graph, const std::vector<std::string>& args) override {
        (void)args; // Unused
        int rank = graph.getRank();
        if (rank == 0) std::cout << "Running Connected Components..." << std::endl;
        
        ConnectedComponents cc(graph);
        auto results = cc.compute();
        
        for (int r = 0; r < graph.getSize(); ++r) {
            if (rank == r) {
                for (VertexId i = 0; i < graph.numLocalVertices(); ++i) {
                    VertexId global_id = graph.globalStartId() + i;
                    std::cout << "V[" << global_id << "]: CC_ID=" << results[i] << std::endl;
                }
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
};
REGISTER_ALGORITHM(CCPlugin);

class PageRankPlugin : public IAlgorithm {
public:
    std::string name() const override { return "pr"; }
    void run(Graph& graph, const std::vector<std::string>& args) override {
        (void)args;
        int rank = graph.getRank();
        if (rank == 0) std::cout << "Running PageRank..." << std::endl;
        
        PageRank pr(graph);
        auto results = pr.compute();
        
        for (int r = 0; r < graph.getSize(); ++r) {
            if (rank == r) {
                for (VertexId i = 0; i < graph.numLocalVertices(); ++i) {
                    VertexId global_id = graph.globalStartId() + i;
                    std::cout << "V[" << global_id << "]: PR=" << std::fixed << std::setprecision(4) << results[i] << std::endl;
                }
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
};
REGISTER_ALGORITHM(PageRankPlugin);

class LPAPlugin : public IAlgorithm {
public:
    std::string name() const override { return "lpa"; }
    void run(Graph& graph, const std::vector<std::string>& args) override {
        (void)args;
        int rank = graph.getRank();
        if (rank == 0) std::cout << "Running Label Propagation..." << std::endl;
        
        LabelPropagation lpa(graph);
        auto results = lpa.compute();
        
        for (int r = 0; r < graph.getSize(); ++r) {
            if (rank == r) {
                for (VertexId i = 0; i < graph.numLocalVertices(); ++i) {
                    VertexId global_id = graph.globalStartId() + i;
                    std::cout << "V[" << global_id << "]: Community=" << results[i] << std::endl;
                }
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
};
REGISTER_ALGORITHM(LPAPlugin);

class RWPlugin : public IAlgorithm {
public:
    std::string name() const override { return "rw"; }
    void run(Graph& graph, const std::vector<std::string>& args) override {
        int walk_len = 10;
        int num_walks = 5;
        if (args.size() >= 1) walk_len = std::stoi(args[0]);
        if (args.size() >= 2) num_walks = std::stoi(args[1]);
        
        int rank = graph.getRank();
        if (rank == 0) std::cout << "Running Random Walk (L=" << walk_len << ", N=" << num_walks << ")..." << std::endl;
        
        RandomWalk rw(graph);
        rw.compute(walk_len, num_walks, "walks_out");
        
        if (rank == 0) std::cout << "Random Walks written to walks_out_*.txt" << std::endl;
    }
};
REGISTER_ALGORITHM(RWPlugin);

} // namespace dgraph
