#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ROOT 0
#define MSG_TAG 100
#define ITERATIONS 10000000 // 1e8
#define PI 3.1415926535897932384626433832795028841971

static const long double pi = PI;

int is_random_inside() {
  long double x = (long double)rand() / RAND_MAX;
  long double y = (long double)rand() / RAND_MAX;
  // return sqrt(x * x + y * y) <= 1.0L;
  return x * x + y * y <= 1.0L; // With R = 1, sqrt is not necessary.
}

int main(int argc, char **argv) {
  int size, rank;

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  srand(rank + time(NULL));

  if (rank == ROOT) {
    MPI_Status status;
    long double inside = 0;
    long double total = 0;
    long double mypi = 0;
    int res;

    while (1) {
      MPI_Recv(&res, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
               &status);

      printf("Received data from [%d] with %d / %d inside\n", status.MPI_SOURCE,
             res, ITERATIONS);
      inside += (long double)res;
      total += (long double)ITERATIONS;

      mypi = inside / total * 4.0L;

      printf("Total inside: %.1Lf | Total points: %.1Lf\n", inside, total);
      printf("My PI:   %.20Lf\n", mypi);
      printf("Real PI: %.20Lf\n", pi);
      if (pi - mypi < 0) {
        printf("Diff:   %.20Lf\n", pi - mypi);
      } else {
        printf("Diff:    %.20Lf\n", pi - mypi);
      }
    }
  } else {
    while (1) {
      int inside = 0;
      int i;
      for (i = 0; i < ITERATIONS; ++i) {
        if (is_random_inside()) {
          ++inside;
        }
      }
      // pewnie jakiś for tutaj
      MPI_Send(&inside, 1, MPI_INT, ROOT, MSG_TAG, MPI_COMM_WORLD);
    }
  }

  MPI_Finalize();
}
