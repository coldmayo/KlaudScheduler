#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Calculating Pi using Monte Carlo method
// It works by drawing a square with a quarter circle inside of it
// Then spawn random points then use the formula: pi = \frac{4 * in}{total points} to find pi
// This formula becomes: pi = \frac{4 * global}{points_inside_per_proc * # of processes}

int main(int argc. char ** argv) {
	int rank, size;
	int in = 0; int out = 0;
	int num_points = 100000000;
	int local_count = 0;   // number of points that a single MPI process found to be inside the unit circle
	int global_count = 0;   // total number of points inside the circle from all processes combined
	double x, y;

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if (argc > 1) {
		num_points = atol(argv[1]);
	}

	unsigned int seed = (unsigned int)(time(NULL) + rank);

	for (i = 0; i < num_points; i++) {
		x = (double)rand_r(&seed) / RAND_MAX;
		y = (double)rand_r(&seed) / RAND_MAX;
		if (x*x + y*y <= 1.0) {
			local_count++;
		}
	}

	MPI_Reduce(&local_count, &global_count, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

	if (rank == 0) {
		float pi = 4.0*(double)global_count / total_points;
		printf("Estimated Pi = %f\nWith %d processes:", pi, size);
	}

	MPI_Finalize();
	return 0;
}
