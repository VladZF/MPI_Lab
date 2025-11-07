#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

    double *full_A = NULL, *full_x = NULL, *full_y = NULL;
    int *counts = NULL, *displs = NULL;

    if (my_rank == ROOT_PROCESS) {
        full_A = (double*)malloc(M * N * sizeof(double));
        full_x = (double*)malloc(N * sizeof(double));
        full_y = (double*)malloc(M * sizeof(double));
        srand(time(NULL));
        for(int i=0; i<M*N; i++) full_A[i] = (double)rand() / RAND_MAX;
        for(int i=0; i<N; i++) full_x[i] = (double)rand() / RAND_MAX;

        counts = (int*)malloc(comm_sz * sizeof(int));
        displs = (int*)malloc(comm_sz * sizeof(int));
        int base_cols = N / comm_sz;
        int remainder = N % comm_sz;
        int offset = 0;
        for (int i = 0; i < comm_sz; i++) {
            counts[i] = base_cols + (i < remainder ? 1 : 0);
            displs[i] = offset;
            offset += counts[i];
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    int local_N;
    MPI_Scatter(counts, 1, MPI_INT, &local_N, 1, MPI_INT, ROOT_PROCESS, MPI_COMM_WORLD);

    double *local_x = (double*)malloc(local_N * sizeof(double));
    MPI_Scatterv(full_x, counts, displs, MPI_DOUBLE,
                 local_x, local_N, MPI_DOUBLE,
                 ROOT_PROCESS, MPI_COMM_WORLD);

    double *local_A = (double*)malloc(M * local_N * sizeof(double));
    if (my_rank == ROOT_PROCESS) {
        for (int p = 1; p < comm_sz; p++) {
            MPI_Datatype col_block_type;
            MPI_Type_vector(M, counts[p], N, MPI_DOUBLE, &col_block_type);
            MPI_Type_commit(&col_block_type);
            
            MPI_Send(&full_A[displs[p]], 1, col_block_type, p, 0, MPI_COMM_WORLD);
            
            MPI_Type_free(&col_block_type);
        }
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < local_N; j++) {
                local_A[i * local_N + j] = full_A[i * N + j];
            }
        }
    } else {
        MPI_Recv(local_A, M * local_N, MPI_DOUBLE, ROOT_PROCESS, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    double *partial_y = (double*)calloc(M, sizeof(double));
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < local_N; j++) {
            partial_y[i] += local_A[i * local_N + j] * local_x[j];
        }
    }

    MPI_Reduce(partial_y, full_y, M, MPI_DOUBLE, MPI_SUM, ROOT_PROCESS, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();
    double local_time = end_time - start_time;
    double max_time;
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, ROOT_PROCESS, MPI_COMM_WORLD);

    if (my_rank == ROOT_PROCESS) {
        printf("%d,%d,%d,%f\n", comm_sz, M, N, max_time);

        free(full_A);
        free(full_x);
        free(full_y);
        free(counts);
        free(displs);
    }
    free(local_x);
    free(local_A);
    free(partial_y);

    MPI_Finalize();
    return 0;
}