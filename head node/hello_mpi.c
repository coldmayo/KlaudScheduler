#include <mpi.h>
#include <stdio.h>

int main(int argc, char ** argv) {
	printf("Starting MPI\n");
	MPI_Init(&argc, &argv);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	printf("Hello from process %d of %d\n", world_rank, world_size);
	fflush(stdout);
	MPI_Finalize();
	return 0;
}
