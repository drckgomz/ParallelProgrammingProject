#ifndef SELECTION_H
#define SELECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

typedef int elem_t;

/* Utility functions (utils.c) */
void fill_random_array(elem_t *a, int n, int seed);
void copy_array(const elem_t *src, elem_t *dst, int n);
int verify_kth_smallest(const elem_t *a, int n, int k, elem_t candidate);
double elapsed_seconds(void);

/* Serial helpers */
int serial_partition(elem_t *a, int n, elem_t pivot,
                     int *n_less, int *n_equal);
int serial_select(elem_t *a, int n, int k);

/* Median-of-medians pivot (sequential) */
elem_t mom_pivot(elem_t *a, int n);

#endif
