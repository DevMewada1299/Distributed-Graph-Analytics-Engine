#pragma once

#include <cstdint>
#include <vector>
#include <iostream>

namespace dgraph {

using VertexId = uint64_t;
using EdgeWeight = float;
using RankId = int;

struct Edge {
    VertexId src;
    VertexId dst;
    EdgeWeight weight;
};

} // namespace dgraph
