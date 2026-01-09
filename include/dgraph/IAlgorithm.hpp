#pragma once

#include "Graph.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>

namespace dgraph {

// Interface for all algorithms
class IAlgorithm {
public:
    virtual ~IAlgorithm() = default;
    
    // Returns the name of the algorithm (CLI key)
    virtual std::string name() const = 0;
    
    // Run the algorithm
    // args: Command line arguments passed after the algorithm name
    virtual void run(Graph& graph, const std::vector<std::string>& args) = 0;
};

// Factory for registering and creating algorithms
class AlgorithmRegistry {
public:
    static AlgorithmRegistry& instance() {
        static AlgorithmRegistry instance;
        return instance;
    }

    void registerAlgorithm(std::unique_ptr<IAlgorithm> algo) {
        if (algo) {
            algorithms_[algo->name()] = std::move(algo);
        }
    }

    IAlgorithm* getAlgorithm(const std::string& name) {
        auto it = algorithms_.find(name);
        if (it != algorithms_.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    const std::map<std::string, std::unique_ptr<IAlgorithm>>& getAll() const {
        return algorithms_;
    }

private:
    std::map<std::string, std::unique_ptr<IAlgorithm>> algorithms_;
};

// Helper macro for static registration
#define REGISTER_ALGORITHM(AlgoClass) \
    static struct AlgoClass##Registrar { \
        AlgoClass##Registrar() { \
            dgraph::AlgorithmRegistry::instance().registerAlgorithm(std::make_unique<AlgoClass>()); \
        } \
    } AlgoClass##_registrar;

} // namespace dgraph
