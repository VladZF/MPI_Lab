#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mpi.h"

#define ROOT_PROCESS 0

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int my_rank, comm_sz;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    if (argc < 3) {
        if (my_rank == ROOT_PROCESS) fprintf(stderr, "Использование: %s <строки> <столбцы>\n", argv[0]);
        MPI_Finalize(); return 1;
    }
    const int M = atoi(argv[1]);
    const int N = atoi(argv[2]);

    double *full_A = NULL;
    double *full_y = NULL;
    double *x = (double*)malloc(N * sizeof(double));

    int *gather_counts = NULL;
    int *gather_displs = NULL;

    if (my_rank == ROOT_PROCESS) {
        full_A = (double*)malloc(M * N * sizeof(double));
        full_y = (double*)malloc(M * sizeof(double));
        srand(time(NULL));
        for(int i=0; i<M*N; i++) full_A[i] = (double)rand() / RAND_MAX;
        for(int i=0; i<N; i++) x[i] = (double)rand() / RAND_MAX;

        gather_counts = (int*)malloc(comm_sz * sizeof(int));
        gather_displs = (int*)malloc(comm_sz * sizeof(int));

        int base_rows = M / comm_sz;
        int remainder = M % comm_sz;
        int offset = 0;

        for (int i = 0; i < comm_sz; i++) {
            gather_counts[i] = base_rows + (i < remainder ? 1 : 0);
            gather_displs[i] = offset;
            offset += gather_counts[i];
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    MPI_Bcast(x, N, MPI_DOUBLE, ROOT_PROCESS, MPI_COMM_WORLD);

    int local_M;
    MPI_Scatter(gather_counts, 1, MPI_INT, &local_M, 1, MPI_INT, ROOT_PROCESS, MPI_COMM_WORLD);

    double *local_A = (double*)malloc(local_M * N * sizeof(double));

    int *scatter_counts = NULL;
    int *scatter_displs = NULL;
    if (my_rank == ROOT_PROCESS) {
        scatter_counts = (int*)malloc(comm_sz * sizeof(int));
        scatter_displs = (int*)malloc(comm_sz * sizeof(int));
        for (int i = 0; i < comm_sz; i++) {
            scatter_counts[i] = gather_counts[i] * N;
            scatter_displs[i] = gather_displs[i] * N;
        }
    }
    MPI_Scatterv(full_A, scatter_counts, scatter_displs, MPI_DOUBLE,
                 local_A, local_M * N, MPI_DOUBLE,
                 ROOT_PROCESS, MPI_COMM_WORLD);
    if (my_rank == ROOT_PROCESS) {
        free(scatter_counts);
        free(scatter_displs);
    }

    double *local_y = (double*)calloc(local_M, sizeof(double));
    for (int i = 0; i < local_M; i++) {
        for (int j = 0; j < N; j++) {
            local_y[i] += local_A[i * N + j] * x[j];
        }
    }

    MPI_Gatherv(local_y, local_M, MPI_DOUBLE,
                full_y, gather_counts, gather_displs, MPI_DOUBLE,
                ROOT_PROCESS, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();
    double local_time = end_time - start_time;
    double max_time;
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, ROOT_PROCESS, MPI_COMM_WORLD);

    if (my_rank == ROOT_PROCESS) {
        printf("%d,%d,%d,%f\n", comm_sz, M, N, max_time);

        free(full_A);
        free(full_y);
        free(gather_counts);
        free(gather_displs);
    }
    free(x);
    free(local_A);
    free(local_y);

    MPI_Finalize();
    return 0;
}