#include <mpi.h>
#include <iostream>
using namespace std;
int main(int argc, char **argv)
{
	int size,rank;
	MPI::Init(argc,argv);

	size=MPI::COMM_WORLD.Get_size();
	rank=MPI::COMM_WORLD.Get_rank();

	cout <<"Hello world: " << rank <<" of " << size << endl;
	MPI::Finalize();
}
