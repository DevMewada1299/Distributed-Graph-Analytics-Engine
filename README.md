# Distributed Graph Analytics Engine

A high-performance, distributed graph processing engine built with C++17, designed to execute complex graph algorithms like PageRank, BFS, and Community Detection on large-scale datasets. It features a modular architecture supporting **MPI** for distributed memory communication and **OpenMP** for shared-memory parallelism, along with an interactive web-based visualization tool and Machine Learning integration.

## System Architecture
```mermaid
flowchart TD
    subgraph INPUT["📥 Input Layer"]
        F1["Edge List File\n(.txt / .csv)"]
        F2["User Upload\n(Web UI)"]
        F3["CLI Arguments\n(algorithm, params)"]
    end

    subgraph GRAPH["🗂️ Graph Layer — Graph.cpp / Graph.hpp"]
        G1["Graph Loader\nParse edge list"]
        G2["1D Partitioning\nDistribute vertices\nacross MPI ranks"]
        G3["CSR Format\nCompressed Sparse Row\nin-memory representation"]
    end

    subgraph MPI["🌐 Distributed Layer — MPI"]
        M1["MPI Rank 0\n(Master)"]
        M2["MPI Rank 1"]
        M3["MPI Rank N"]
        M4["MPI_Allreduce\nGlobal aggregation\n(e.g. dangling mass)"]
        M5["Mock MPI\nSingle-node fallback"]
    end

    subgraph OMP["⚡ Shared Memory — OpenMP"]
        O1["Intra-node\nMulti-threading"]
        O2["Parallel loops\nover local vertices"]
    end

    subgraph ENGINE["⚙️ BSP Engine — Engine.hpp"]
        E1["Superstep Loop"]
        E2["SCATTER\nSend messages\nto neighbors"]
        E3["COMMUNICATE\nMPI inter-rank\nmessage exchange"]
        E4["GATHER\nCollect incoming\nmessages per vertex"]
        E5["APPLY\nUpdate vertex state\nvia algorithm logic"]
        E6["Convergence Check\nΔ < threshold\nor max iterations"]
    end

    subgraph REGISTRY["🔌 Algorithm Registry — IAlgorithm.hpp"]
        R1["AlgorithmRegistry\nPlugin Discovery"]
        R2["REGISTER_ALGORITHM\nMacro"]
        R3["UserAlgorithms.hpp\nPlugin Entry Point"]
    end

    subgraph ALGOS["📐 Algorithm Implementations"]
        A1["PageRank\nNode importance\nMPI_Allreduce for\ndangling mass"]
        A2["Label Propagation\nCommunity Detection\nMajority neighbor label"]
        A3["BFS\nShortest Path\nLevel-synchronous"]
        A4["Connected Components\nMin-ID propagation"]
        A5["Random Walk\nGraph sampling\nfor embeddings"]
    end

    subgraph ML["🤖 ML Layer"]
        ML1["Random Walk Output\nwalks_out_*.txt"]
        ML2["train_embeddings.py\nNode2Vec / GraphSAGE"]
        ML3["Node Embeddings\n64-dim vectors\nembeddings.txt"]
        ML4["Downstream\nML Tasks"]
    end

    subgraph VIZ["🖥️ Visualization Layer"]
        V1["Flask Server\nviz/app.py\nport 5001"]
        V2["Vis.js Web UI\nindex.html"]
        V3["Interactive Graph\nEdit nodes/edges"]
        V4["Algorithm Runner\nSelect algo + params"]
        V5["Dynamic Styling\nResize/recolor nodes\nby result"]
    end

    subgraph OUTPUT["📤 Output Layer"]
        OUT1["CLI stdout\nRanks / distances / labels"]
        OUT2["PR Comments\n(if CI integrated)"]
        OUT3["Visual Graph\nin browser"]
    end

    %% Input → Graph
    F1 & F2 --> G1
    F3 --> E1
    G1 --> G2 --> G3

    %% Graph → MPI ranks
    G3 --> M1 & M2 & M3
    M1 <-->|"MPI Send/Recv"| M2
    M2 <-->|"MPI Send/Recv"| M3
    M1 & M2 & M3 --> M4
    M5 -.->|"dev/test fallback"| M1

    %% MPI + OMP → Engine
    M1 & M4 --> E1
    O1 --> O2 --> E2

    %% BSP Superstep cycle
    E1 --> E2 --> E3 --> E4 --> E5 --> E6
    E6 -->|"not converged"| E1
    E6 -->|"converged"| OUT1

    %% Engine → Registry → Algorithms
    E5 --> R1
    R1 --> R2 --> R3
    R3 --> A1 & A2 & A3 & A4 & A5

    %% Algorithms → ML
    A5 --> ML1 --> ML2 --> ML3 --> ML4

    %% Flask ↔ Engine
    V1 -->|"subprocess call"| E1
    V2 --> V3 & V4
    V4 --> V1
    E6 -->|"JSON results"| V1
    V1 --> V5 --> OUT3

    %% File upload path
    F2 --> V1

    %% Styling
    classDef inputStyle fill:#0e1a2b,stroke:#00e5ff,color:#00e5ff
    classDef graphStyle fill:#0d1f18,stroke:#10b981,color:#10b981
    classDef mpiStyle fill:#1a1030,stroke:#7c3aed,color:#a78bfa
    classDef ompStyle fill:#1f1800,stroke:#f59e0b,color:#fbbf24
    classDef engineStyle fill:#0f1f2b,stroke:#38bdf8,color:#38bdf8
    classDef algoStyle fill:#1a0f2b,stroke:#e879f9,color:#e879f9
    classDef mlStyle fill:#0d1f18,stroke:#34d399,color:#34d399
    classDef vizStyle fill:#1f1200,stroke:#fb923c,color:#fb923c
    classDef outputStyle fill:#1f0d0d,stroke:#f87171,color:#f87171
    classDef registryStyle fill:#1a1a0d,stroke:#d4d400,color:#d4d400

    class F1,F2,F3 inputStyle
    class G1,G2,G3 graphStyle
    class M1,M2,M3,M4,M5 mpiStyle
    class O1,O2 ompStyle
    class E1,E2,E3,E4,E5,E6 engineStyle
    class A1,A2,A3,A4,A5 algoStyle
    class ML1,ML2,ML3,ML4 mlStyle
    class V1,V2,V3,V4,V5 vizStyle
    class OUT1,OUT2,OUT3 outputStyle
    class R1,R2,R3 registryStyle
```

## 🚀 Key Features

*   **Distributed Computing**: Implements a vertex-centric programming model (BSP - Bulk Synchronous Parallel) over MPI.
*   **Hybrid Parallelism**: Combines MPI for inter-node communication and OpenMP for intra-node multi-threading.
*   **Comprehensive Algorithms**:
    *   **PageRank**: For node importance ranking.
    *   **Label Propagation**: For fast community detection.
    *   **Breadth-First Search (BFS)**: For shortest path analysis.
    *   **Connected Components (CC)**: For finding disjoint subgraphs.
    *   **Random Walk**: For generating graph embeddings (Node2Vec/GraphSAGE).
*   **Machine Learning Ready**: Includes tools to train node embeddings (Node2Vec) from graph structure for downstream ML tasks.
*   **Interactive Visualization**: A Flask + Vis.js web interface to visualize graph structures, run algorithms interactively, and edit graphs in real-time.
*   **Extensible Plugin System**: Easily add custom algorithms without modifying the core engine code.
*   **Mock MPI Support**: Includes a mock MPI implementation for seamless development and testing on single-node machines.

---

## 🏗️ Architecture

The system is divided into two main components: the high-performance C++ Backend and the Interactive Frontend.

### 1. C++ Backend (The Engine)
*   **`Graph` ([Graph.hpp](include/dgraph/Graph.hpp))**: Loads and partitions the graph across multiple MPI ranks (1D partitioning, CSR format).
*   **`Engine` ([Engine.hpp](include/dgraph/Engine.hpp))**: Orchestrates the BSP supersteps (Scatter -> Communicate -> Gather -> Apply).
*   **`AlgorithmRegistry` ([IAlgorithm.hpp](include/dgraph/IAlgorithm.hpp))**: Manages algorithm discovery and execution via a plugin system.

### 2. Visualization Frontend
*   **Flask Server ([app.py](viz/app.py))**: Acts as a bridge between the web UI and the C++ binary. It parses graph files, accepts user inputs (algorithm selection, parameters), and executes the engine.
*   **Vis.js UI**: Renders the graph in the browser. Supports:
    *   **Interactive Editing**: Add/remove nodes and edges.
    *   **Algorithm Selection**: Choose between PageRank, BFS, Community Detection, etc.
    *   **Dynamic Styling**: Nodes resize/recolor based on analysis results.

---

## 📂 Directory Structure

```
.
├── CMakeLists.txt          # Build configuration
├── src/
│   ├── main.cpp            # Entry point (CLI & Algorithm Runner)
│   └── Graph.cpp           # Graph loading logic
├── include/
│   ├── dgraph/
│   │   ├── Graph.hpp
│   │   ├── Engine.hpp
│   │   ├── IAlgorithm.hpp  # Algorithm Interface
│   │   ├── algorithms/     # Algorithm Implementations (BFS, PR, CC, etc.)
│   │   └── plugins/        # Plugin Registration (UserAlgorithms.hpp)
├── viz/
│   ├── app.py              # Flask server
│   └── templates/
│       └── index.html      # Web UI
├── scripts/
│   └── train_embeddings.py # ML training script (Node2Vec)
└── data/                   # Input datasets
```

---

## 🛠️ Build & Installation

### Prerequisites
*   **C++ Compiler**: GCC or Clang (C++17 support)
*   **CMake**: Version 3.14+
*   **Python 3**: For visualization and ML tools
*   **(Optional) MPI**: OpenMPI or MPICH. *Falls back to Mock MPI if missing.*

### 1. Build the C++ Engine
```bash
mkdir -p build && cd build
cmake ..
make
```
This produces the `dgraph_engine` executable in `build/`.

### 2. Set up Python Environment
```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

---

## 🏃 Usage

### 1. Command Line Interface (CLI)

Run the engine directly on a dataset:

```bash
# Syntax: ./dgraph_engine <graph_file> [algorithm] [params...]

# PageRank (Default)
./build/dgraph_engine data/social_network.txt pr

# Breadth-First Search (Source Node = 0)
./build/dgraph_engine data/social_network.txt bfs 0

# Connected Components
./build/dgraph_engine data/social_network.txt cc

# Random Walk (Length=10, Walks=5)
./build/dgraph_engine data/social_network.txt rw 10 5
```

### 2. Interactive Visualization

1.  **Start the Server**:
    ```bash
    source venv/bin/activate
    python3 viz/app.py
    ```
2.  **Open Browser**: Go to `http://127.0.0.1:5001`.
3.  **Features**:
    *   **Upload**: Upload your own edge list file.
    *   **Edit**: Click "Edit" to modify the graph visually.
    *   **Run**: Select an algorithm (e.g., BFS), enter parameters (e.g., Source Node), and click "Run Analysis".

### 3. Machine Learning (Graph Embeddings)

You can generate node embeddings (like Node2Vec) to use in downstream ML tasks.

1.  **Generate Walks**:
    ```bash
    ./build/dgraph_engine data/social_network.txt rw 10 5
    ```
    This creates `walks_out_*.txt` files.

2.  **Train Embeddings**:
    ```bash
    python3 scripts/train_embeddings.py --walks "walks_out_*.txt" --output embeddings.txt --dim 64
    ```

---

## 🔌 Custom Extensions

You can add your own algorithms using the Plugin System without modifying the core engine.

1.  Create your algorithm class inheriting from `dgraph::IAlgorithm`.
2.  Register it with `REGISTER_ALGORITHM`.
3.  Include it in `include/dgraph/plugins/UserAlgorithms.hpp`.

See **[README_EXTENSIONS.md](README_EXTENSIONS.md)** for a detailed tutorial.

---

## 🧪 Algorithm Details

*   **PageRank**: Measures node importance. Uses `MPI_Allreduce` for dangling node mass redistribution.
*   **Label Propagation**: Fast community detection. Nodes adopt the majority label of neighbors.
*   **BFS**: Computes shortest path distance from a source. Uses level-synchronous expansion.
*   **Connected Components**: Propagates smallest node ID to find disjoint sets.
*   **Random Walk**: Simulates random walkers for sampling graph structure.
