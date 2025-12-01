CC     = gcc
MPICC  = mpicc
NVCC   = nvcc

CFLAGS   = -O2 -Wall -Iinclude -fopenmp
LDFLAGS  = -fopenmp
MPICFLAGS = -O2 -Wall -Iinclude
NVCCFLAGS = -O2 -Iinclude

SRC_COMMON = src/utils.c
OBJ_COMMON = $(SRC_COMMON:.c=.o)

all: quickselect_omp momselect_omp quickselect_mpi momselect_mpi selection_cuda

# Shared object for utils
src/%.o: src/%.c include/selection.h
	$(CC) $(CFLAGS) -c $< -o $@

quickselect_omp: src/quickselect_omp.c $(OBJ_COMMON)
	$(CC) $(CFLAGS) -o $@ src/quickselect_omp.c $(OBJ_COMMON) $(LDFLAGS)

momselect_omp: src/momselect_omp.c $(OBJ_COMMON)
	$(CC) $(CFLAGS) -o $@ src/momselect_omp.c $(OBJ_COMMON) $(LDFLAGS)

quickselect_mpi: src/quickselect_mpi.c $(OBJ_COMMON)
	$(MPICC) $(MPICFLAGS) -o $@ src/quickselect_mpi.c $(OBJ_COMMON)

momselect_mpi: src/momselect_mpi.c $(OBJ_COMMON)
	$(MPICC) $(MPICFLAGS) -o $@ src/momselect_mpi.c $(OBJ_COMMON)

selection_cuda: src/selection_cuda.cu src/utils.c include/selection.h
	$(NVCC) $(NVCCFLAGS) -o $@ src/selection_cuda.cu src/utils.c

clean:
	rm -f src/*.o *.o quickselect_omp momselect_omp quickselect_mpi momselect_mpi selection_cuda
