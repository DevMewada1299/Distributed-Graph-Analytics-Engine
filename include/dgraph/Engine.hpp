#pragma once

#include "Graph.hpp"
#include <vector>
#include "MPI_Wrapper.hpp"
#include <functional>
#include <cstring>
#include <algorithm>

namespace dgraph {

// A simple message structure for graph updates
template <typename T>
struct Message {
    VertexId dst;
    T value;
};

template <typename MsgT, typename AccT = MsgT>
class Engine {
public:
    Engine(Graph& graph) : graph_(graph) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comm_);
        MPI_Comm_rank(comm_, &rank_);
        MPI_Comm_size(comm_, &size_);
    }

    ~Engine() {
        MPI_Comm_free(&comm_);
    }

    // Determine owner of a global vertex
    int getOwner(VertexId vid) const {
        VertexId total = graph_.numGlobalVertices();
        VertexId remainder = total % size_;
        VertexId chunk = total / size_;
        
        VertexId split_point = remainder * (chunk + 1);
        if (vid < split_point) {
            return vid / (chunk + 1);
        } else {
            return remainder + (vid - split_point) / chunk;
        }
    }

    // Synchronize messages
    void syncMessages(const std::vector<std::vector<Message<MsgT>>>& send_buffers,
                      std::vector<Message<MsgT>>& received_messages) {
        
        std::vector<int> send_counts(size_);
        std::vector<int> recv_counts(size_);
        
        for (int i = 0; i < size_; ++i) {
            send_counts[i] = send_buffers[i].size() * sizeof(Message<MsgT>);
        }

        MPI_Alltoall(send_counts.data(), 1, MPI_INT,
                     recv_counts.data(), 1, MPI_INT, comm_);

        std::vector<int> sdispls(size_, 0);
        std::vector<int> rdispls(size_, 0);
        int total_recv_bytes = 0;
        
        std::vector<uint8_t> send_flat;
        
        for (int i = 0; i < size_; ++i) {
            sdispls[i] = send_flat.size();
            const uint8_t* raw_ptr = reinterpret_cast<const uint8_t*>(send_buffers[i].data());
            send_flat.insert(send_flat.end(), raw_ptr, raw_ptr + send_counts[i]);
            
            rdispls[i] = total_recv_bytes;
            total_recv_bytes += recv_counts[i];
        }

        std::vector<uint8_t> recv_flat(total_recv_bytes);

        MPI_Alltoallv(send_flat.data(), send_counts.data(), sdispls.data(), MPI_BYTE,
                      recv_flat.data(), recv_counts.data(), rdispls.data(), MPI_BYTE, comm_);

        int num_msgs = total_recv_bytes / sizeof(Message<MsgT>);
        received_messages.resize(num_msgs);
        if (num_msgs > 0) {
            std::memcpy(received_messages.data(), recv_flat.data(), total_recv_bytes);
        }
    }

    // Run a vertex-centric program
    void run(int iterations, 
             std::function<void(VertexId, std::vector<std::vector<Message<MsgT>>>&)> scatter_func,
             std::function<void(AccT&, const MsgT&)> reduce_func,
             std::function<void(VertexId, const AccT&)> apply_func) {
        
        for (int iter = 0; iter < iterations; ++iter) {
            std::vector<std::vector<Message<MsgT>>> send_buffers(size_);
            
            #pragma omp parallel
            {
                std::vector<std::vector<Message<MsgT>>> thread_local_buffers(size_);
                
                #pragma omp for nowait
                for (VertexId i = 0; i < graph_.numLocalVertices(); ++i) {
                     scatter_func(i, thread_local_buffers);
                }
                
                #pragma omp critical
                {
                    for(int r=0; r<size_; ++r) {
                        send_buffers[r].insert(send_buffers[r].end(), 
                                             thread_local_buffers[r].begin(), 
                                             thread_local_buffers[r].end());
                    }
                }
            }

            std::vector<Message<MsgT>> received_msgs;
            syncMessages(send_buffers, received_msgs);

            std::sort(received_msgs.begin(), received_msgs.end(), 
                      [](const Message<MsgT>& a, const Message<MsgT>& b) { return a.dst < b.dst; });
            
            VertexId current_dst = -1;
            AccT accumulator; // Default constructed
            bool first = true;
            bool has_data = false;

            for (const auto& msg : received_msgs) {
                if (msg.dst != current_dst) {
                    if (!first && has_data) {
                         apply_func(current_dst, accumulator);
                    }
                    current_dst = msg.dst;
                    accumulator = AccT(); // Reset
                    reduce_func(accumulator, msg.value);
                    first = false;
                    has_data = true;
                } else {
                    reduce_func(accumulator, msg.value);
                }
            }
            if (!first && has_data) {
                apply_func(current_dst, accumulator);
            }
        }
    }

    int getRank() const { return rank_; }

private:
    Graph& graph_;
    MPI_Comm comm_;
    int rank_;
    int size_;
};

} // namespace dgraph
