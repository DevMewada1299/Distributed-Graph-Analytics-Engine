#include "dgraph/MPI_Wrapper.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include "dgraph/Graph.hpp"
#include "dgraph/IAlgorithm.hpp"
#include "dgraph/plugins/BuiltinAlgorithms.hpp"
#include "dgraph/plugins/UserAlgorithms.hpp"

int main(int argc, char** argv) {
    // 1. Initialize MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0) {
            std::cerr << "Usage: " << argv[0] << " <graph_file> [algorithm] [params...]" << std::endl;
            std::cerr << "Available Algorithms: ";
            auto& registry = dgraph::AlgorithmRegistry::instance().getAll();
            for (const auto& pair : registry) {
                std::cerr << pair.first << " ";
            }
            std::cerr << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    std::string filename = argv[1];
    std::string algo_name = (argc >= 3) ? argv[2] : "default";
    
    std::vector<std::string> algo_args;
    for(int i=3; i<argc; ++i) algo_args.push_back(argv[i]);

    try {
        // 2. Load Graph
        dgraph::Graph graph(MPI_COMM_WORLD);
        if (rank == 0) std::cout << "Loading graph from " << filename << "..." << std::endl;
        graph.loadFromFile(filename);

        // 3. Run Algorithm
        if (algo_name == "default") {
            // Backward compatibility: Run PR and LPA
             if (rank == 0) std::cout << "Running default suite (PR + LPA)..." << std::endl;
             
             auto* pr = dgraph::AlgorithmRegistry::instance().getAlgorithm("pr");
             if (pr) pr->run(graph, {});
             
             auto* lpa = dgraph::AlgorithmRegistry::instance().getAlgorithm("lpa");
             if (lpa) lpa->run(graph, {});
        } else {
            auto* algo = dgraph::AlgorithmRegistry::instance().getAlgorithm(algo_name);
            if (algo) {
                algo->run(graph, algo_args);
            } else {
                if (rank == 0) std::cerr << "Unknown algorithm: " << algo_name << std::endl;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error on Rank " << rank << ": " << e.what() << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Finalize();
    return 0;
}
