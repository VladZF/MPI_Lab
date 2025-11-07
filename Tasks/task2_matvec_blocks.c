#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ROOT_PROCESS 0

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (argc < 3) {
        if (world_rank == ROOT_PROCESS) fprintf(stderr, "Использование: %s <строки> <столбцы>\n", argv[0]);
        MPI_Finalize(); return 1;
    }
    const int M = atoi(argv[1]);
    const int N = atoi(argv[2]);

    MPI_Comm grid_comm;
    int dims[2] = {0, 0};
    MPI_Dims_create(world_size, 2, dims);

    if (M % dims[0] != 0 || N % dims[1] != 0) {
        if (world_rank == ROOT_PROCESS) fprintf(stderr, "ОШИБКА: Размеры матрицы (%dx%d) не делятся на сетку (%dx%d).\n", M, N, dims[0], dims[1]);
        MPI_Finalize(); return 1;
    }

    int periods[2] = {0, 0};
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &grid_comm);
    int my_grid_rank;
    MPI_Comm_rank(grid_comm, &my_grid_rank);
    int my_coords[2];
    MPI_Cart_coords(grid_comm, my_grid_rank, 2, my_coords);

    MPI_Comm row_comm, col_comm;
    MPI_Comm_split(grid_comm, my_coords[0], my_coords[1], &row_comm);
    MPI_Comm_split(grid_comm, my_coords[1], my_coords[0], &col_comm);

    double *full_A = NULL, *full_x = NULL, *full_y = NULL;
    int local_M = M / dims[0];
    int local_N = N / dims[1];

    if (my_grid_rank == ROOT_PROCESS) {
        full_A = (double*)malloc(M * N * sizeof(double));
        full_x = (double*)malloc(N * sizeof(double));
        full_y = (double*)malloc(M * sizeof(double));
        srand(time(NULL));
        for(int i=0; i<M*N; i++) full_A[i] = (double)rand() / RAND_MAX;
        for(int i=0; i<N; i++) full_x[i] = (double)rand() / RAND_MAX;
    }

    MPI_Barrier(grid_comm);
    double start_time = MPI_Wtime();

    double *local_x = (double*)malloc(local_N * sizeof(double));
    if (my_coords[0] == 0) {
        int *col_counts = (int*)malloc(dims[1] * sizeof(int));
        int *col_displs = (int*)malloc(dims[1] * sizeof(int));
        for(int i=0; i<dims[1]; i++) { col_counts[i] = local_N; col_displs[i] = i * local_N; }

        MPI_Scatterv(full_x, col_counts, col_displs, MPI_DOUBLE, local_x, local_N, MPI_DOUBLE, ROOT_PROCESS, row_comm);

        free(col_counts); free(col_displs);
    }
    MPI_Bcast(local_x, local_N, MPI_DOUBLE, ROOT_PROCESS, col_comm);


    double *local_A = (double*)malloc(local_M * local_N * sizeof(double));

    MPI_Datatype block_type, resized_block_type;
    if (my_grid_rank == ROOT_PROCESS) {
        MPI_Type_vector(local_M, local_N, N, MPI_DOUBLE, &block_type);
        MPI_Type_create_resized(block_type, 0, sizeof(double), &resized_block_type);
        MPI_Type_commit(&resized_block_type);
        MPI_Type_free(&block_type);
    }

    int *scatter_counts = NULL, *scatter_displs = NULL;
    if (my_grid_rank == ROOT_PROCESS) {
        scatter_counts = (int*)malloc(world_size * sizeof(int));
        scatter_displs = (int*)malloc(world_size * sizeof(int));
        for (int i = 0; i < dims[0]; i++) {
            for (int j = 0; j < dims[1]; j++) {
                int rank;
                int coords[] = {i, j};
                MPI_Cart_rank(grid_comm, coords, &rank);
                scatter_counts[rank] = 1;
                scatter_displs[rank] = i * local_M * N + j * local_N;
            }
        }
    }

    MPI_Scatterv(full_A, scatter_counts, scatter_displs, resized_block_type,
                 local_A, local_M * local_N, MPI_DOUBLE,
                 ROOT_PROCESS, grid_comm);

    double *partial_y = (double*)calloc(local_M, sizeof(double));
    for (int i = 0; i < local_M; i++) {
        for (int j = 0; j < local_N; j++) {
            partial_y[i] += local_A[i * local_N + j] * local_x[j];
        }
    }

    double *row_reduced_y = NULL;
    if (my_coords[1] == 0) {
        row_reduced_y = (double*)malloc(local_M * sizeof(double));
    }
    MPI_Reduce(partial_y, row_reduced_y, local_M, MPI_DOUBLE, MPI_SUM, ROOT_PROCESS, row_comm);

    if (my_coords[1] == 0) {
        int *row_counts = (int*)malloc(dims[0] * sizeof(int));
        int *row_displs = (int*)malloc(dims[0] * sizeof(int));
        for(int i=0; i<dims[0]; i++) { row_counts[i] = local_M; row_displs[i] = i * local_M; }

        MPI_Gatherv(row_reduced_y, local_M, MPI_DOUBLE,
                    full_y, row_counts, row_displs, MPI_DOUBLE,
                    ROOT_PROCESS, col_comm);
        free(row_counts); free(row_displs);
    }

    double end_time = MPI_Wtime();
    double local_time = end_time - start_time;
    double max_time;
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, grid_comm);

    if (my_grid_rank == ROOT_PROCESS) {
        printf("%d,%d,%d,%f\n", world_size, M, N, max_time);
        free(full_A); free(full_x); free(full_y);
        free(scatter_counts); free(scatter_displs);
        MPI_Type_free(&resized_block_type);
    }
    if (my_coords[1] == 0) free(row_reduced_y);

    free(local_A); free(local_x); free(partial_y);
    MPI_Comm_free(&row_comm); MPI_Comm_free(&col_comm); MPI_Comm_free(&grid_comm);

    MPI_Finalize();
    return 0;
}