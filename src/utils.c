#define _POSIX_C_SOURCE 199309L
#include "selection.h"
#include <string.h>
#include <time.h>

/* Fill array with random integers */
void fill_random_array(elem_t *a, int n, int seed) {
    srand(seed);
    for (int i = 0; i < n; i++) {
        a[i] = rand();  // optionally: rand() % 1000000;
    }
}

void copy_array(const elem_t *src, elem_t *dst, int n) {
    memcpy(dst, src, n * sizeof(elem_t));
}

static int cmp_int(const void *x, const void *y) {
    elem_t a = *(const elem_t *)x;
    elem_t b = *(const elem_t *)y;
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

/* Verify that candidate is the k-th smallest (0-based) */
int verify_kth_smallest(const elem_t *a, int n, int k, elem_t candidate) {
    if (k < 0 || k >= n) return 0;
    elem_t *tmp = (elem_t *)malloc(n * sizeof(elem_t));
    if (!tmp) return 0;
    memcpy(tmp, a, n * sizeof(elem_t));
    qsort(tmp, n, sizeof(elem_t), cmp_int);
    int ok = (tmp[k] == candidate);
    free(tmp);
    return ok;
}

/* Simple global-ish timer (seconds since first call) */
double elapsed_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    static long long base_sec = -1;
    if (base_sec < 0) base_sec = ts.tv_sec;
    return (double)(ts.tv_sec - base_sec) + ts.tv_nsec / 1e9;
}

/* ===== Serial helpers ===== */

/* Partition a[0..n-1] around pivot.
   Returns new size = n.
   n_less, n_equal are returned via pointers.
   Layout after partition: [ < pivot ][ == pivot ][ > pivot ]. */
int serial_partition(elem_t *a, int n, elem_t pivot,
                     int *n_less, int *n_equal) {
    int lt = 0, eq = 0, gt = 0;
    for (int i = 0; i < n; i++) {
        if (a[i] < pivot) lt++;
        else if (a[i] == pivot) eq++;
        else gt++;
    }

    elem_t *tmp = (elem_t *)malloc(n * sizeof(elem_t));
    int idx_lt = 0, idx_eq = lt, idx_gt = lt + eq;
    for (int i = 0; i < n; i++) {
        if (a[i] < pivot) tmp[idx_lt++] = a[i];
        else if (a[i] == pivot) tmp[idx_eq++] = a[i];
        else tmp[idx_gt++] = a[i];
    }
    memcpy(a, tmp, n * sizeof(elem_t));
    free(tmp);

    if (n_less)  *n_less  = lt;
    if (n_equal) *n_equal = eq;
    return n;
}

/* Simple serial Quickselect */
int serial_select(elem_t *a, int n, int k) {
    int left = 0, right = n - 1;
    while (1) {
        if (left == right) return a[left];

        int pivot_index = left + rand() % (right - left + 1);
        elem_t pivot = a[pivot_index];

        int i = left, lt = left, gt = right;
        while (i <= gt) {
            if (a[i] < pivot) {
                elem_t tmp = a[lt]; a[lt] = a[i]; a[i] = tmp;
                lt++; i++;
            } else if (a[i] > pivot) {
                elem_t tmp = a[gt]; a[gt] = a[i]; a[i] = tmp;
                gt--;
            } else {
                i++;
            }
        }

        if (k < lt) {
            right = lt - 1;
        } else if (k > gt) {
            left = gt + 1;
        } else {
            return pivot;
        }
    }
}

/* ===== Median-of-medians pivot ===== */

static void insertion_sort5(elem_t *a, int start, int len) {
    for (int i = start + 1; i < start + len; i++) {
        elem_t key = a[i];
        int j = i - 1;
        while (j >= start && a[j] > key) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

/* Median-of-medians (BFPRT) pivot selection.
   Modifies array a[0..n-1] during computation. */
elem_t mom_pivot(elem_t *a, int n) {
    if (n <= 5) {
        insertion_sort5(a, 0, n);
        return a[n / 2];
    }

    int num_groups = (n + 4) / 5;
    for (int g = 0; g < num_groups; g++) {
        int start = g * 5;
        int len = (start + 5 <= n) ? 5 : (n - start);
        insertion_sort5(a, start, len);
        int median_idx = start + len / 2;
        elem_t tmp = a[g];
        a[g] = a[median_idx];
        a[median_idx] = tmp;
    }
    return mom_pivot(a, num_groups);
}
