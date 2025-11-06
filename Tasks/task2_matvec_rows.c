#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mpi.h"

void init_matrix(double* matrix, int rows, int cols) {
    for (int i = 0; i < rows * cols; i++) {
        matrix[i] = (double)(i % 100);
    }
}

void init_vector(double* vector, int size) {
    for (int i = 0; i < size; i++) {
        vector[i] = (double)(i % 100);
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int my_rank, comm_sz;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    if (argc != 3) {
        if (my_rank == 0) {
            fprintf(stderr, "Использование: %s <строки> <столбцы>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    const int M = atoi(argv[1]);
    const int N = atoi(argv[2]);

    if (M % comm_sz != 0) {
        if (my_rank == 0) {
            fprintf(stderr, "ОШИБКА: Количество строк (%d) должно делиться нацело на количество процессов (%d).\n", M, comm_sz);
        }
        MPI_Finalize();
        return 1;
    }

    double *full_A = NULL;
    double *x = (double*)malloc(N * sizeof(double));

    if (my_rank == 0) {
        srand(time(NULL));
        full_A = (double*)malloc(M * N * sizeof(double));
        init_matrix(full_A, M, N);
        init_vector(x, N);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    const int local_M = M / comm_sz;
    double *local_A = (double*)malloc(local_M * N * sizeof(double));
    double *full_y = NULL;

    MPI_Scatter(full_A, local_M * N, MPI_DOUBLE, local_A, local_M * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(x, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double *local_y = (double*)malloc(local_M * sizeof(double));
    for (int i = 0; i < local_M; i++) {
        local_y[i] = 0.0;
        for (int j = 0; j < N; j++) {
            local_y[i] += local_A[i * N + j] * x[j];
        }
    }

    if (my_rank == 0) {
        full_y = (double*)malloc(M * sizeof(double));
    }
    MPI_Gather(local_y, local_M, MPI_DOUBLE, full_y, local_M, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();
    double local_time = end_time - start_time;
    double max_time;
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        printf("%d,%d,%d,%f\n", comm_sz, M, N, max_time);
        free(full_A);
        free(full_y);
    }

    free(local_A);
    free(local_y);
    free(x);

    MPI_Finalize();
    return 0;
}