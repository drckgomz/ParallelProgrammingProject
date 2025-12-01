#include "selection.h"
#include <omp.h>
#include <string.h>

/* Same parallel partition as quickselect_omp.c
   (you can refactor to a shared file if you want). */
static int parallel_partition(elem_t *a, int n, elem_t pivot,
                              int *n_less, int *n_equal, int num_threads)
{
    if (n < 10000 || num_threads <= 1) {
        return serial_partition(a, n, pivot, n_less, n_equal);
    }

    int *less_counts  = (int *)calloc(num_threads, sizeof(int));
    int *equal_counts = (int *)calloc(num_threads, sizeof(int));

    #pragma omp parallel num_threads(num_threads)
    {
        int tid = omp_get_thread_num();
        int start = (n * tid) / num_threads;
        int end   = (n * (tid + 1)) / num_threads;
        int lc = 0, ec = 0;
        for (int i = start; i < end; i++) {
            if (a[i] < pivot) lc++;
            else if (a[i] == pivot) ec++;
        }
        less_counts[tid]  = lc;
        equal_counts[tid] = ec;
    }

    int total_less = 0, total_equal = 0;
    for (int i = 0; i < num_threads; i++) {
        total_less  += less_counts[i];
        total_equal += equal_counts[i];
    }

    int *less_offset  = (int *)malloc(num_threads * sizeof(int));
    int *equal_offset = (int *)malloc(num_threads * sizeof(int));
    less_offset[0] = 0;
    equal_offset[0] = 0;
    for (int i = 1; i < num_threads; i++) {
        less_offset[i]  = less_offset[i - 1]  + less_counts[i - 1];
        equal_offset[i] = equal_offset[i - 1] + equal_counts[i - 1];
    }

    elem_t *tmp = (elem_t *)malloc(n * sizeof(elem_t));

    #pragma omp parallel num_threads(num_threads)
    {
        int tid = omp_get_thread_num();
        int start = (n * tid) / num_threads;
        int end   = (n * (tid + 1)) / num_threads;

        int lt_idx = less_offset[tid];
        int eq_idx = total_less + equal_offset[tid];

        for (int i = start; i < end; i++) {
            if (a[i] < pivot) {
                tmp[lt_idx++] = a[i];
            } else if (a[i] == pivot) {
                tmp[eq_idx++] = a[i];
            }
        }
    }

    int gt_start = total_less + total_equal;
    int gt_idx   = gt_start;
    for (int i = 0; i < n; i++) {
        if (a[i] > pivot) {
            tmp[gt_idx++] = a[i];
        }
    }

    memcpy(a, tmp, n * sizeof(elem_t));

    free(less_counts);
    free(equal_counts);
    free(less_offset);
    free(equal_offset);
    free(tmp);

    if (n_less)  *n_less  = total_less;
    if (n_equal) *n_equal = total_equal;
    return n;
}

/* Deterministic median-of-medians selection using OpenMP */
elem_t parallel_momselect_omp(elem_t *a, int n, int k, int num_threads) {
    int left = 0;
    int right = n - 1;

    while (1) {
        int active_n = right - left + 1;
        if (active_n <= 20000) {
            return serial_select(a + left, active_n, k - left);
        }

        elem_t *segment = a + left;
        elem_t pivot = mom_pivot(segment, active_n);

        int n_less = 0, n_equal = 0;
        parallel_partition(segment, active_n, pivot,
                           &n_less, &n_equal, num_threads);

        int less_start    = left;
        int equal_start   = left + n_less;
        int greater_start = left + n_less + n_equal;

        if (k < equal_start) {
            right = equal_start - 1;
        } else if (k < greater_start) {
            return pivot;
        } else {
            left = greater_start;
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <n> <k> <num_threads> [seed]\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int k = atoi(argv[2]);   // 0-based
    int num_threads = atoi(argv[3]);
    int seed = (argc >= 5) ? atoi(argv[4]) : (int)time(NULL);

    elem_t *a = (elem_t *)malloc(n * sizeof(elem_t));
    elem_t *backup = (elem_t *)malloc(n * sizeof(elem_t));
    if (!a || !backup) {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    fill_random_array(a, n, seed);
    copy_array(a, backup, n);
    srand(seed);

    double t0 = elapsed_seconds();
    elem_t result = parallel_momselect_omp(a, n, k, num_threads);
    double t1 = elapsed_seconds();

    printf("[OMP Median-of-Medians] n=%d k=%d threads=%d\n", n, k, num_threads);
    printf("Result: %d\n", result);
    printf("Time: %f seconds\n", t1 - t0);

    int ok = verify_kth_smallest(backup, n, k, result);
    printf("Verification: %s\n", ok ? "OK" : "FAILED\n");

    free(a);
    free(backup);
    return ok ? 0 : 2;
}
