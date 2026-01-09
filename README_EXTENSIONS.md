# Extensions Guide

## 1. Custom Algorithms (Plugin System)

You can add your own graph algorithms without modifying the core engine code.

### Steps:
1.  Create a new header file (e.g., `include/dgraph/algorithms/MyAlgo.hpp`).
2.  Implement your algorithm class. You can use the `Engine` class for BSP processing.
3.  Create a "Plugin" wrapper class that inherits from `dgraph::IAlgorithm`.
4.  Register it using `REGISTER_ALGORITHM(MyPluginClass)`.
5.  Include your file in `include/dgraph/plugins/UserAlgorithms.hpp`.

### Example `MyAlgo.hpp`:
```cpp
#pragma once
#include "../Graph.hpp"
#include "../Engine.hpp"
#include "../IAlgorithm.hpp"

namespace dgraph {

class MyAlgo {
    // ... Implementation using Engine ...
};

class MyAlgoPlugin : public IAlgorithm {
public:
    std::string name() const override { return "myalgo"; }
    
    void run(Graph& graph, const std::vector<std::string>& args) override {
        // Parse args and run MyAlgo
    }
};

REGISTER_ALGORITHM(MyAlgoPlugin);

}
```

### Usage:
Recompile the project. You can now run:
```bash
./dgraph_engine mygraph.txt myalgo [args...]
```

## 2. Machine Learning Integration (Node2Vec)

The engine supports generating random walks for Node2Vec/GraphSAGE training.

### Step 1: Generate Walks
Run the engine with the `rw` (Random Walk) algorithm:
```bash
# Generate 5 walks of length 10 per node
./dgraph_engine data/social_network.txt rw 10 5
```
This produces `walks_out_0.txt`, `walks_out_1.txt`, etc.

### Step 2: Train Embeddings
Use the provided Python script to train embeddings:
```bash
python3 scripts/train_embeddings.py --walks "walks_out_*.txt" --output embeddings.txt --dim 64
```

### Step 3: Use in ML
Load `embeddings.txt` into PyTorch/TensorFlow or Scikit-Learn for downstream tasks like node classification or link prediction.
