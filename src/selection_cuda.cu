// src/selection_cuda.cu

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/copy.h>
#include <thrust/nth_element.h>

extern "C" {
#include "selection.h"
}

struct CudaTimer {
    cudaEvent_t start, stop;
    CudaTimer() {
        cudaEventCreate(&start);
        cudaEventCreate(&stop);
    }
    ~CudaTimer() {
        cudaEventDestroy(start);
        cudaEventDestroy(stop);
    }
    void tic() {
        cudaEventRecord(start);
    }
    float toc() {
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
        float ms = 0.0f;
        cudaEventElapsedTime(&ms, start, stop);
        return ms;
    }
};

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <n> <k> [seed]\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int k = atoi(argv[2]); // 0-based
    int seed = (argc >= 4) ? atoi(argv[3]) : (int)time(NULL);

    elem_t *h_data   = (elem_t *)malloc(n * sizeof(elem_t));
    elem_t *h_backup = (elem_t *)malloc(n * sizeof(elem_t));
    if (!h_data || !h_backup) {
        fprintf(stderr, "Host allocation failed\n");
        return 1;
    }

    fill_random_array(h_data, n, seed);
    copy_array(h_data, h_backup, n);

    thrust::host_vector<elem_t> h_vec(h_data, h_data + n);
    thrust::device_vector<elem_t> d_vec = h_vec;

    CudaTimer timer;
    timer.tic();

    thrust::nth_element(d_vec.begin(), d_vec.begin() + k, d_vec.end());

    float ms = timer.toc();

    elem_t result = d_vec[k];

    printf("[CUDA Thrust nth_element] n=%d k=%d\n", n, k);
    printf("Result: %d\n", result);
    printf("GPU time: %f ms\n", ms);

    int ok = verify_kth_smallest(h_backup, n, k, result);
    printf("Verification: %s\n", ok ? "OK" : "FAILED\n");

    free(h_data);
    free(h_backup);
    return ok ? 0 : 2;
}
