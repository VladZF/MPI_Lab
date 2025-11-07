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
        if (my_rank == 0) fprintf(stderr, "Использование: %s <строки> <столбцы>\n", argv[0]);
        MPI_Finalize(); return 1;
    }

    const int M = atoi(argv[1]);
    const int N = atoi(argv[2]);

    if (N % comm_sz != 0) {
        if (my_rank == 0) fprintf(stderr, "ОШИБКА: Количество столбцов (%d) должно делиться нацело на количество процессов (%d).\n", N, comm_sz);
        MPI_Finalize(); return 1;
    }

    double *full_A = NULL;
    double *full_x = NULL;
    double *full_y = NULL;

    if (my_rank == 0) {
        srand(time(NULL));
        full_A = (double*)malloc(M * N * sizeof(double));
        full_x = (double*)malloc(N * sizeof(double));
        full_y = (double*)malloc(M * sizeof(double));
        init_matrix(full_A, M, N);
        init_vector(full_x, N);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    const int local_N = N / comm_sz;
    double *local_A = (double*)malloc(M * local_N * sizeof(double));
    double *local_x = (double*)malloc(local_N * sizeof(double));

    MPI_Scatter(full_x, local_N, MPI_DOUBLE, local_x, local_N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    MPI_Datatype col_type, resized_col_type;
    MPI_Type_vector(M, 1, N, MPI_DOUBLE, &col_type);
    MPI_Type_commit(&col_type);
    
    MPI_Type_create_resized(col_type, 0, 1 * sizeof(double), &resized_col_type);
    MPI_Type_commit(&resized_col_type);

    MPI_Scatter(full_A, local_N, resized_col_type, local_A, M * local_N, MPI_DOUBLE, 0, MPI_COMM_WORLD);


    double *partial_y = (double*)calloc(M, sizeof(double));
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < local_N; j++) {
            partial_y[i] += local_A[i * local_N + j] * local_x[j];
        }
    }

    MPI_Reduce(partial_y, full_y, M, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();
    double local_time = end_time - start_time;
    double max_time;
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        printf("%d,%d,%d,%f\n", comm_sz, M, N, max_time);
        free(full_A);
        free(full_x);
        free(full_y);
    }

    MPI_Type_free(&col_type);
    MPI_Type_free(&resized_col_type);
    free(local_A);
    free(local_x);
    free(partial_y);

    MPI_Finalize();
    return 0;
}