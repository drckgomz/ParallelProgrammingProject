#include "selection.h"
#include <mpi.h>
#include <string.h>

/* Partition + compact locally into <, ==, > segments */
static void local_compact(elem_t *a, int n, elem_t pivot,
                          elem_t **new_a, int *new_n,
                          int *local_less, int *local_equal)
{
    int lt = 0, eq = 0;
    for (int i = 0; i < n; i++) {
        if (a[i] < pivot) lt++;
        else if (a[i] == pivot) eq++;
    }
    int gt = n - lt - eq;

    elem_t *tmp = (elem_t *)malloc(n * sizeof(elem_t));
    int idx_lt = 0, idx_eq = lt, idx_gt = lt + eq;
    for (int i = 0; i < n; i++) {
        if (a[i] < pivot)      tmp[idx_lt++] = a[i];
        else if (a[i] == pivot) tmp[idx_eq++] = a[i];
        else                    tmp[idx_gt++] = a[i];
    }

    free(a);
    *new_a = tmp;
    *new_n = n;
    *local_less  = lt;
    *local_equal = eq;
}

/* Parallel randomized selection in MPI */
elem_t parallel_quickselect_mpi(elem_t **a_ptr, int *n_ptr,
                                long long k_global, MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    elem_t *a = *a_ptr;
    int n_local = *n_ptr;

    long long global_n;
    MPI_Allreduce(&n_local, &global_n, 1, MPI_LONG_LONG, MPI_SUM, comm);

    while (1) {
        if (global_n <= 50000) {
            /* Small enough: gather all to rank 0 and finish serially */
            int *recvcounts = NULL;
            int *displs     = NULL;
            elem_t *all     = NULL;

            if (rank == 0) {
                recvcounts = (int *)malloc(size * sizeof(int));
                displs     = (int *)malloc(size * sizeof(int));
            }

            MPI_Gather(&n_local, 1, MPI_INT,
                       recvcounts, 1, MPI_INT,
                       0, comm);

            int total = 0;
            if (rank == 0) {
                for (int i = 0; i < size; i++) {
                    displs[i] = total;
                    total += recvcounts[i];
                }
                all = (elem_t *)malloc(total * sizeof(elem_t));
            }

            MPI_Gatherv(a, n_local, MPI_INT,
                        all, recvcounts, displs, MPI_INT,
                        0, comm);

            elem_t ans = 0;
            if (rank == 0) {
                ans = serial_select(all, total, (int)k_global);
            }
            MPI_Bcast(&ans, 1, MPI_INT, 0, comm);

            if (rank == 0) {
                free(recvcounts);
                free(displs);
                free(all);
            }
            return ans;
        }

        /* Random pivot: each process proposes, root picks one */
        elem_t local_pivot = 0;
        if (n_local > 0) {
            local_pivot = a[rand() % n_local];
        }
        elem_t *pivots = NULL;
        if (rank == 0) pivots = (elem_t *)malloc(size * sizeof(elem_t));

        MPI_Gather(&local_pivot, 1, MPI_INT,
                   pivots, 1, MPI_INT,
                   0, comm);

        elem_t pivot = 0;
        if (rank == 0) {
            pivot = pivots[rand() % size];
            free(pivots);
        }
        MPI_Bcast(&pivot, 1, MPI_INT, 0, comm);

        /* Local partition */
        int local_less = 0, local_equal = 0;
        local_compact(a, n_local, pivot, &a, &n_local,
                      &local_less, &local_equal);

        long long global_less, global_equal;
        MPI_Allreduce(&local_less,  &global_less,  1, MPI_LONG_LONG, MPI_SUM, comm);
        MPI_Allreduce(&local_equal, &global_equal, 1, MPI_LONG_LONG, MPI_SUM, comm);

        long long less_end  = global_less;
        long long equal_end = global_less + global_equal;

        if (k_global < less_end) {
            /* Keep < pivot */
            elem_t *new_a = (elem_t *)malloc(local_less * sizeof(elem_t));
            for (int i = 0; i < local_less; i++) new_a[i] = a[i];
            free(a);
            a = new_a;
            n_local = local_less;
            global_n = global_less;
        } else if (k_global < equal_end) {
            /* Pivot is answer */
            *a_ptr = a;
            *n_ptr = n_local;
            return pivot;
        } else {
            /* Keep > pivot */
            long long new_k = k_global - equal_end;
            int local_gt = n_local - local_less - local_equal;
            elem_t *new_a = (elem_t *)malloc(local_gt * sizeof(elem_t));
            for (int i = 0; i < local_gt; i++) {
                new_a[i] = a[local_less + local_equal + i];
            }
            free(a);
            a = new_a;
            n_local = local_gt;
            global_n -= (global_less + global_equal);
            k_global = new_k;
        }
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0)
            fprintf(stderr, "Usage: %s <n> <k> [seed]\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    int n = atoi(argv[1]);
    int k = atoi(argv[2]);   // 0-based
    int seed = (argc >= 4) ? atoi(argv[3]) : (int)time(NULL);

    int base = n / size;
    int rem  = n % size;
    int local_n = base + (rank < rem ? 1 : 0);

    elem_t *local_a = (elem_t *)malloc(local_n * sizeof(elem_t));

    if (rank == 0) {
        elem_t *global_a = (elem_t *)malloc(n * sizeof(elem_t));
        fill_random_array(global_a, n, seed);

        int *sendcounts = (int *)malloc(size * sizeof(int));
        int *displs     = (int *)malloc(size * sizeof(int));
        int offset = 0;
        for (int i = 0; i < size; i++) {
            sendcounts[i] = base + (i < rem ? 1 : 0);
            displs[i]     = offset;
            offset        += sendcounts[i];
        }

        MPI_Scatterv(global_a, sendcounts, displs, MPI_INT,
                     local_a, local_n, MPI_INT,
                     0, MPI_COMM_WORLD);

        free(global_a);
        free(sendcounts);
        free(displs);
    } else {
        MPI_Scatterv(NULL, NULL, NULL, MPI_INT,
                     local_a, local_n, MPI_INT,
                     0, MPI_COMM_WORLD);
    }

    srand(seed + rank);   // each rank gets different RNG

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = elapsed_seconds();
    elem_t res = parallel_quickselect_mpi(&local_a, &local_n, k, MPI_COMM_WORLD);
    double t1 = elapsed_seconds();

    if (rank == 0) {
        printf("[MPI Quickselect] n=%d k=%d procs=%d\n", n, k, size);
        printf("Result: %d\n", res);
        printf("Time: %f seconds\n", t1 - t0);
        printf("Verification: (compare with OMP or CUDA run using same seed)\n");
    }

    free(local_a);
    MPI_Finalize();
    return 0;
}
