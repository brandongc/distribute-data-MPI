#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "bins.h"

long random_at_mostL(long max) {
  unsigned long num_bins = (unsigned long) max + 1, num_rand = (unsigned long) RAND_MAX + 1, bin_size = num_rand / num_bins, defect = num_rand % num_bins;
  long x;
  do {
    x = random();
  }
  while (num_rand - defect <= (unsigned long)x);
  return x/bin_size;
}

char* readable_fs(double size/*in bytes*/, char *buf) {
  int i = 0;
  const char* units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
  while (size > 1024) {
    size /= 1024;
    i++;
  }
  sprintf(buf, "%.*f %s", i, size, units[i]);
  return buf;
}

/*  Only effective if N is much smaller than RAND_MAX */
void shuffle(int *array, size_t n) {
  if (n > 1) {
    size_t i;
    for (i = 0; i < n - 1; i++) {
      size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
      int t = array[j];
      array[j] = array[i];
      array[i] = t;
    }
  }
}

int main(int argc, char** argv) {
  char buf[20];

  int nrows;  // A( nrows X ncols )
  int ncols;
  int krows;   // B( krows X ncols )
  int ngroups; // number of worker groups

  int i,j;
  int rank, nprocs;

  double *A, *B;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  srand(rank);

  if (rank == 0) {
    
    if (argc != 5) {
      printf("Usage: %s Arows Acols Brows Bgroups\n", argv[0]);
      fflush(stdout);
      MPI_Abort(MPI_COMM_WORLD, 1);
    } else {
      nrows = atoi(argv[1]);
      ncols = atoi(argv[2]);
      krows = atoi(argv[3]);
      ngroups = atoi(argv[4]);
    }

    size_t sizeA = (size_t) nrows * (size_t) ncols * sizeof(double);
    size_t sizeB = (size_t) ngroups * (size_t) krows * (size_t) ncols * sizeof(double);

    printf("Total A: %s\n", readable_fs((double) sizeA, buf));
    printf("Total B: %s\n", readable_fs((double) sizeB, buf));
    printf("Total:   %s\n", readable_fs((double) (sizeA+sizeB), buf));

    printf("A per rank: %s\n", readable_fs( (double) sizeA / (double) nprocs , buf));
    printf("B per rank: %s\n", readable_fs( (double) sizeB / (double) nprocs , buf));

    printf("Num procs: %i\n", nprocs);
    printf("B groups: %i\n\n", ngroups);
    printf("A dimensions: (%i, %i)\n", nrows, ncols);
    printf("B dimensions: (%i, %i)\n", krows, ncols);
      
    if ( nprocs > nrows ) {
      printf("must have nprocs < nrows \n");
      fflush(stdout);
      MPI_Abort(MPI_COMM_WORLD, 3);
    }

    if ( ngroups > nprocs ) {
      printf("must have ngroups < nprocs \n");
      fflush(stdout);
      MPI_Abort(MPI_COMM_WORLD, 4);
    }   

  }

  MPI_Bcast(&nrows, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&ncols, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&krows, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&ngroups, 1, MPI_INT, 0, MPI_COMM_WORLD);
     
  int local_rows = bin_size_1D(rank, nrows, nprocs);
  
  size_t sizeA = (size_t) local_rows * (size_t) ncols * sizeof(double);  
  A = (double *) malloc(sizeA);
  memset(A, rank, sizeA);

  MPI_Win win;
  MPI_Win_create(A, sizeA, sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

  int color = bin_coord_1D(rank, nprocs, ngroups);  
  MPI_Comm comm_g;
  MPI_Comm_split(MPI_COMM_WORLD, color, rank, &comm_g);
  
  int nprocs_g, rank_g;
  MPI_Comm_size(comm_g, &nprocs_g);
  MPI_Comm_rank(comm_g, &rank_g);

  int qcols = ncols;
  int qrows = bin_size_1D(rank_g, krows, nprocs_g);
  size_t sizeB = (size_t) qcols * (size_t) qrows * sizeof(double);    
  B = (double *) malloc(sizeB);

#ifndef SIMPLESAMPLE
  int *sample;
  if (rank_g == 0) {
    sample = malloc( nrows * sizeof(int) );
    for (i=0; i<nrows; i++) sample[i]=i;
    shuffle(sample, nrows);
  } else {
    sample = NULL;
  }

  int srows[qrows];

  {
    int sendcounts[nprocs_g];
    int displs[nprocs_g];

    for (i=0; i<nprocs_g; i++) {
      int ubound;
      bin_range_1D(i, krows, nprocs_g, &displs[i], &ubound);
      sendcounts[i] = bin_size_1D(i, krows, nprocs_g);
    }
   
    MPI_Scatterv(sample, sendcounts, displs, MPI_INT, &srows, qrows, MPI_INT, 0, comm_g);
    
    if (rank_g == 0) free(sample);
  }
#endif

  double t = MPI_Wtime();
  MPI_Win_fence(MPI_MODE_NOPUT | MPI_MODE_NOPRECEDE, win);

  for (i=0; i<qrows; i++) {
#ifdef SIMPLESAMPLE
    int trow = (int) random_at_mostL( (long) nrows);
#else
    int trow = srows[i];
#endif
    int target_rank = bin_coord_1D(trow, nrows, nprocs);
    int target_disp = bin_index_1D(trow, nrows, nprocs) * ncols;
    MPI_Get( &B[i*qcols], qcols, MPI_DOUBLE, target_rank, target_disp, qcols, MPI_DOUBLE, win);
  }

  MPI_Win_fence(MPI_MODE_NOSUCCEED, win);

  double tmax, tcomm = MPI_Wtime() - t;
  MPI_Reduce(&tcomm, &tmax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  if (rank == 0) {
    printf("Comm time: %f (s)\n", tmax);
  }
  
  MPI_Win_free(&win);
  free(A);
  /* do work on B here */
  free(B);
  MPI_Finalize();
  return 0;
}
