#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
	int size,rank, len;
	char processor[100];
	//sleep(2);
	MPI_Init(&argc,&argv);

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Get_processor_name(processor, &len);

	printf("Hello world: %d of %d na (%s)\n", rank, size, processor);
	//sleep(2);
	MPI_Finalize();
}
