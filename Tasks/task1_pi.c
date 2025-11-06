#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mpi.h"

#define HALF_SQUARE_SIZE 1

double GetRandomDouble(const double min, const double max) {
    if (min > max) {
        return min;
    }

    return min + ((double)rand() / RAND_MAX) * (max - min);
}

void GetPi(int comm_sz, double* result, unsigned long long points_count) {
    unsigned long long local_points_count = points_count / comm_sz;
    unsigned long long counter = 0;
    double local_result = 0;
    for (unsigned long long i = 0; i < local_points_count; i++) {
        double x = GetRandomDouble(-HALF_SQUARE_SIZE, HALF_SQUARE_SIZE);
        double y = GetRandomDouble(-HALF_SQUARE_SIZE, HALF_SQUARE_SIZE);
        if (x * x + y * y <= 1) {
            counter++;
        }
    }
    local_result = (double)counter / (double)points_count * 4.0;
    MPI_Reduce(&local_result, result, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
}

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    unsigned long long points_count = strtoull(argv[1], NULL, 10);
    int my_rank;
    int comm_sz;

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    srand(time(NULL) + my_rank);
    
    double parallel_result;
    double common_time;

    MPI_Barrier(MPI_COMM_WORLD);
    
    double start = MPI_Wtime();
    GetPi(comm_sz, &parallel_result, points_count);
    double end = MPI_Wtime();

    double time = end-start;

    MPI_Reduce(&time, &common_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        printf("%d,%lf,%lf\n", comm_sz, parallel_result, common_time);
    }
    
    MPI_Finalize();

    return 0;
}
