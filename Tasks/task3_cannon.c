#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define ROOT_PROCESS 0

void multiply_add(double *local_A, double *local_B, double *local_C, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                local_C[i * n + j] += local_A[i * n + k] * local_B[k * n + j];
            }
        }
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (argc < 2) {
        if (world_rank == ROOT_PROCESS) fprintf(stderr, "Использование: %s <размер_матрицы>\n", argv[0]);
        MPI_Finalize(); return 1;
    }
    const int N = atoi(argv[1]);

    int grid_dim = (int)sqrt(world_size);
    if (grid_dim * grid_dim != world_size) {
        if (world_rank == ROOT_PROCESS) fprintf(stderr, "ОШИБКА: Количество процессов должно быть полным квадратом!\n");
        MPI_Finalize(); return 1;
    }
    if (N % grid_dim != 0) {
        if (world_rank == ROOT_PROCESS) fprintf(stderr, "ОШИБКА: Размер матрицы (%d) должен делиться нацело на размер сетки (%d).\n", N, grid_dim);
        MPI_Finalize(); return 1;
    }

    MPI_Comm grid_comm;
    int dims[2] = {grid_dim, grid_dim};
    int periods[2] = {1, 1};
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &grid_comm);

    int my_grid_rank;
    MPI_Comm_rank(grid_comm, &my_grid_rank);
    int my_coords[2];
    MPI_Cart_coords(grid_comm, my_grid_rank, 2, my_coords);

    double *full_A = NULL, *full_B = NULL, *full_C = NULL;
    int local_N = N / grid_dim;

    if (my_grid_rank == ROOT_PROCESS) {
        full_A = (double*)malloc(N * N * sizeof(double));
        full_B = (double*)malloc(N * N * sizeof(double));
        full_C = (double*)malloc(N * N * sizeof(double));
        srand(time(NULL));
        for(int i=0; i<N*N; i++) {
            full_A[i] = (double)(i % 100);
            full_B[i] = (double)(i % 50);
        }
    }

    double *local_A = (double*)malloc(local_N * local_N * sizeof(double));
    double *local_B = (double*)malloc(local_N * local_N * sizeof(double));

    MPI_Datatype block_type, resized_block_type;
    if (my_grid_rank == ROOT_PROCESS) {
        MPI_Type_vector(local_N, local_N, N, MPI_DOUBLE, &block_type);
        MPI_Type_create_resized(block_type, 0, sizeof(double), &resized_block_type);
        MPI_Type_commit(&resized_block_type);
    }

    int *scatter_counts = NULL, *scatter_displs = NULL;
    if (my_grid_rank == ROOT_PROCESS) {
        scatter_counts = (int*)malloc(world_size * sizeof(int));
        scatter_displs = (int*)malloc(world_size * sizeof(int));
        for (int i=0; i<world_size; i++) scatter_counts[i] = 1;
        int rank = 0;
        for (int i=0; i<grid_dim; i++) {
            for (int j=0; j<grid_dim; j++) {
                scatter_displs[rank++] = i * N * local_N + j * local_N;
            }
        }
    }
    MPI_Scatterv(full_A, scatter_counts, scatter_displs, resized_block_type, local_A, local_N*local_N, MPI_DOUBLE, 0, grid_comm);
    MPI_Scatterv(full_B, scatter_counts, scatter_displs, resized_block_type, local_B, local_N*local_N, MPI_DOUBLE, 0, grid_comm);


    MPI_Barrier(grid_comm);
    double start_time = MPI_Wtime();

    double *local_C = (double*)calloc(local_N * local_N, sizeof(double));
    int left_peer, right_peer, up_peer, down_peer;

    MPI_Cart_shift(grid_comm, 1, -my_coords[0], &right_peer, &left_peer);
    MPI_Sendrecv_replace(local_A, local_N*local_N, MPI_DOUBLE, left_peer, 0, right_peer, 0, grid_comm, MPI_STATUS_IGNORE);

    MPI_Cart_shift(grid_comm, 0, -my_coords[1], &down_peer, &up_peer);
    MPI_Sendrecv_replace(local_B, local_N*local_N, MPI_DOUBLE, up_peer, 1, down_peer, 1, grid_comm, MPI_STATUS_IGNORE);

    for (int i = 0; i < grid_dim; i++) {
        multiply_add(local_A, local_B, local_C, local_N);

        MPI_Cart_shift(grid_comm, 1, -1, &right_peer, &left_peer);
        MPI_Sendrecv_replace(local_A, local_N*local_N, MPI_DOUBLE, left_peer, 0, right_peer, 0, grid_comm, MPI_STATUS_IGNORE);

        MPI_Cart_shift(grid_comm, 0, -1, &down_peer, &up_peer);
        MPI_Sendrecv_replace(local_B, local_N*local_N, MPI_DOUBLE, up_peer, 1, down_peer, 1, grid_comm, MPI_STATUS_IGNORE);
    }

    MPI_Gatherv(local_C, local_N*local_N, MPI_DOUBLE, full_C, scatter_counts, scatter_displs, resized_block_type, 0, grid_comm);

    double end_time = MPI_Wtime();
    double local_time = end_time - start_time;
    double max_time;
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, grid_comm);

    if (my_grid_rank == ROOT_PROCESS) {
        printf("%d,%d,%f\n", world_size, N, max_time);
        free(full_A); free(full_B); free(full_C);
        free(scatter_counts); free(scatter_displs);
        MPI_Type_free(&resized_block_type);
    }
    free(local_A); free(local_B); free(local_C);
    MPI_Comm_free(&grid_comm);

    MPI_Finalize();
    return 0;
}
