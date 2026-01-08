#include "dgraph/MPI_Wrapper.hpp"
#include <iostream>
#include <iomanip>
#include "dgraph/Graph.hpp"
#include "dgraph/algorithms/PageRank.hpp"
#include "dgraph/algorithms/LabelPropagation.hpp"

int main(int argc, char** argv) {
    // 1. Initialize MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0) {
            std::cerr << "Usage: " << argv[0] << " <graph_file>" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    std::string filename = argv[1];

    try {
        // 2. Load Graph
        dgraph::Graph graph(MPI_COMM_WORLD);
        if (rank == 0) std::cout << "Loading graph from " << filename << "..." << std::endl;
        graph.loadFromFile(filename);

        // 3. Run PageRank
        if (rank == 0) std::cout << "\n--- Running PageRank ---" << std::endl;
        dgraph::PageRank pr_algo(graph);
        auto pr_results = pr_algo.compute(10, 0.85);

        // 4. Run Label Propagation
        if (rank == 0) std::cout << "\n--- Running Label Propagation ---" << std::endl;
        dgraph::LabelPropagation lpa_algo(graph);
        auto lpa_results = lpa_algo.compute(5);

        // 5. Output Results (Partial)
        // Each rank prints its own results for its local vertices
        // Synchronize for clean output
        for (int r = 0; r < size; ++r) {
            if (rank == r) {
                std::cout << "Rank " << rank << " results:" << std::endl;
                for (dgraph::VertexId i = 0; i < graph.numLocalVertices(); ++i) {
                    dgraph::VertexId global_id = graph.globalStartId() + i;
                    std::cout << "  V[" << global_id << "]: PR=" << std::fixed << std::setprecision(4) 
                              << pr_results[i] << ", Community=" << lpa_results[i] << std::endl;
                }
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error on Rank " << rank << ": " << e.what() << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Finalize();
    return 0;
}
