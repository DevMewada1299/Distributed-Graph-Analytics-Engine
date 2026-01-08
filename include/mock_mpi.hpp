#pragma once

#include <cstring>
#include <iostream>
#include <vector>

// Mock MPI for single-node testing
typedef int MPI_Comm;
typedef int MPI_Datatype;
typedef int MPI_Op;

#define MPI_COMM_WORLD 0
#define MPI_INT 0
#define MPI_DOUBLE 1
#define MPI_BYTE 2
#define MPI_UINT64_T 3

#define MPI_SUM 0
#define MPI_MAX 1

#define MPI_THREAD_FUNNELED 1

inline int MPI_Init_thread(int* argc, char*** argv, int required, int* provided) {
    (void)argc; (void)argv; (void)required;
    *provided = required;
    return 0;
}

inline int MPI_Finalize() {
    return 0;
}

inline int MPI_Comm_rank(MPI_Comm comm, int* rank) {
    (void)comm;
    *rank = 0;
    return 0;
}

inline int MPI_Comm_size(MPI_Comm comm, int* size) {
    (void)comm;
    *size = 1;
    return 0;
}

inline int MPI_Comm_dup(MPI_Comm comm, MPI_Comm* newcomm) {
    *newcomm = comm;
    return 0;
}

inline int MPI_Comm_free(MPI_Comm* comm) {
    (void)comm;
    return 0;
}

inline int MPI_Barrier(MPI_Comm comm) {
    (void)comm;
    return 0;
}

inline int MPI_Abort(MPI_Comm comm, int errorcode) {
    (void)comm;
    std::cerr << "MPI_Abort called with error " << errorcode << std::endl;
    exit(errorcode);
    return 0;
}

inline int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    (void)buffer; (void)count; (void)datatype; (void)root; (void)comm;
    // For size=1, Bcast does nothing (root is me)
    return 0;
}

inline int MPI_Allreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
    (void)op; (void)comm;
    // For size=1, recv = send
    size_t bytes = 0;
    if (datatype == MPI_INT) bytes = count * sizeof(int);
    else if (datatype == MPI_DOUBLE) bytes = count * sizeof(double);
    else if (datatype == MPI_UINT64_T) bytes = count * sizeof(uint64_t);
    
    std::memcpy(recvbuf, sendbuf, bytes);
    return 0;
}

inline int MPI_Alltoall(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                        void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
    (void)comm; (void)recvcount; (void)recvtype;
    // For size=1, sendbuf -> recvbuf
    // sendcount is elements per rank.
    size_t bytes = 0;
    if (sendtype == MPI_INT) bytes = sendcount * sizeof(int);
    // ... others
    
    std::memcpy(recvbuf, sendbuf, bytes);
    return 0;
}

inline int MPI_Alltoallv(const void* sendbuf, const int* sendcounts, const int* sdispls, MPI_Datatype sendtype,
                         void* recvbuf, const int* recvcounts, const int* rdispls, MPI_Datatype recvtype, MPI_Comm comm) {
    (void)comm; (void)recvcounts; (void)recvtype;
    // For size=1, copy sendbuf[sdispls[0]] to recvbuf[rdispls[0]] with len sendcounts[0]
    // Assuming type matches and is byte for Engine
    if (sendtype == MPI_BYTE) {
        std::memcpy((char*)recvbuf + rdispls[0], (const char*)sendbuf + sdispls[0], sendcounts[0]);
    }
    return 0;
}
