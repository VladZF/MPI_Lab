#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mpi.h"

#define HALF_SQUARE_SIZE 1.0

double GetRandomDouble(const double min, const double max) {
    return min + ((double)rand() / RAND_MAX) * (max - min);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int my_rank, comm_sz;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    if (argc != 2) {
        if (my_rank == 0) {
            fprintf(stderr, "Использование: %s <количество_точек>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }
    unsigned long long total_points = strtoull(argv[1], NULL, 10);

    srand(time(NULL) + my_rank);

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    unsigned long long local_points_count = total_points / comm_sz;

    if (my_rank == 0) {
        local_points_count += total_points % comm_sz;
    }

    unsigned long long local_hits = 0;
    for (unsigned long long i = 0; i < local_points_count; i++) {
        double x = GetRandomDouble(-HALF_SQUARE_SIZE, HALF_SQUARE_SIZE);
        double y = GetRandomDouble(-HALF_SQUARE_SIZE, HALF_SQUARE_SIZE);
        if (x * x + y * y <= 1.0) {
            local_hits++;
        }
    }

    unsigned long long total_hits = 0;
    MPI_Reduce(&local_hits, &total_hits, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    const double end_time = MPI_Wtime();
    const double local_time = end_time - start_time;

    double max_time;
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        double pi_estimate = 4.0 * (double)total_hits / (double)total_points;
        printf("%d,%lf,%lf\n", comm_sz, pi_estimate, max_time);
    }

    MPI_Finalize();
    return 0;
}
