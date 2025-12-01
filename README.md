#   Term Project
### Parallel Programming  
### Derick Gomez  
### Topic: Parallel Selection (k-th Smallest Element)

## 1. Project Overview

This project implements and evaluates parallel algorithms for solving the selection problem, which asks:

**Given an unsorted array of n numbers, find the k-th smallest element.**

Selection is a fundamental problem used in:

- Median and percentile calculations  
- Order statistics  
- Data preprocessing and statistical analysis  
- Real-time systems where full sorting is too slow  

Instead of sorting the entire array (which costs O(n log n)), selection algorithms aim for O(n) time.  
This project extends selection into the parallel computing domain, using three different execution models:

1. **Shared-memory parallelism (OpenMP)**  
2. **Distributed-memory parallelism (MPI)**  
3. **GPU parallelism (CUDA)** – optional extension

The assignment requires two different algorithms, each implemented under OpenMP and MPI.  
A third GPU implementation is included for comparison.

## 2. Algorithms Implemented

### 2.1 Algorithm 1: Randomized Quickselect  

A parallel adaptation of the classic Quickselect algorithm.

**Key idea:**

- Choose a random pivot  
- Partition elements into `< pivot`, `== pivot`, `> pivot`  
- Recurse only into the partition that contains the k-th element  
- Expected linear time due to random pivoting  

### 2.2 Algorithm 2: Deterministic Median-of-Medians  

A selection algorithm with worst-case linear time.

**Key idea:**

1. Split the array into groups of 5  
2. Compute the median of each group  
3. Recursively compute the median of these medians  
4. Use this “median-of-medians" as a pivot  
5. Partition and recurse just like Quickselect  

### 2.3 Optional Algorithm 3: CUDA GPU Selection  

Implemented using Thrust's `nth_element`.

Provides an additional performance comparison across CPU and GPU architectures.

## 3. Directory Structure

ParallelProgrammingProject/
├── README.md
├── Makefile
├── include/
│   └── selection.h
└── src/
    ├── utils.c
    ├── quickselect_omp.c
    ├── momselect_omp.c
    ├── quickselect_mpi.c
    ├── momselect_mpi.c
    └── selection_cuda.cu

## 4. Building the Project

### 4.1 Requirements

- GCC with OpenMP support (or Homebrew GCC on macOS)  
- OpenMPI (mpicc, mpirun)  
- CUDA Toolkit for GPU version (optional)

### 4.2 Build Commands

Linux / Fox cluster:

    module load gcc openmpi
    make

macOS (OpenMP/MPI only):

    brew install gcc open-mpi
    make CC=gcc-14

## 5. Running the Programs

OpenMP:

    ./quickselect_omp <n> <k> <threads> [seed]
    ./momselect_omp   <n> <k> <threads> [seed]

MPI:

    mpirun -np <procs> ./quickselect_mpi <n> <k> [seed]
    mpirun -np <procs> ./momselect_mpi   <n> <k> [seed]

CUDA (optional):

    ./selection_cuda <n> <k> [seed]

## 6. File Descriptions

selection.h - Shared definitions and function prototypes  
utils.c - Random generation, timers, verification, serial helpers  
quickselect_omp.c - Randomized Quickselect with OpenMP  
momselect_omp.c - Deterministic Median-of-Medians with OpenMP  
quickselect_mpi.c - Randomized Quickselect with MPI  
momselect_mpi.c - Deterministic Median-of-Medians with MPI  
selection_cuda.cu - GPU-based selection (optional)

## 7. What This Project Demonstrates

- Shared-memory and distributed-memory parallelism  
- Differences in randomized vs deterministic selection strategies  
- MPI communication patterns: broadcast, allreduce, gather, scatter  
- Performance comparison across CPU and GPU architectures  
- Scalability analysis through execution time and speedup plots  
