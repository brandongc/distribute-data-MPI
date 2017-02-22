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

    if ( nprocs / ngroups > ncols) {
      printf("must have nprocs / ngroups < ncols \n");
      fflush(stdout);
      MPI_Abort(MPI_COMM_WORLD, 2);
    }
      
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
  
#ifdef MPIALLOC
  MPI_Alloc_mem(sizeA, MPI_INFO_NULL, &A);  
#else
  A = (double *) malloc(sizeA);
#endif

  memset(A, rank, sizeA);

  MPI_Win win;
  MPI_Win_create(A, sizeA, sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

  int color = bin_coord_1D(rank, nprocs, ngroups);
  MPI_Comm comm_g;
  MPI_Comm_split(MPI_COMM_WORLD, color, rank, &comm_g);
  
  int nprocs_g, rank_g;
  MPI_Comm_size(comm_g, &nprocs_g);
  MPI_Comm_rank(comm_g, &rank_g);

#ifdef BCOLDIST
  int qcols = bin_size_1D(rank_g, ncols, nprocs_g);
  int qrows = krows;
#else
  int qcols = ncols;
  int qrows = bin_size_1D(rank_g, krows, nprocs_g);
#endif

  size_t sizeB = (size_t) qcols * (size_t) qrows * sizeof(double);
    
#ifdef MPIALLOC
  MPI_Alloc_mem(sizeB, MPI_INFO_NULL, &B);   
#else  
  B = (double *) malloc(sizeB);
#endif
  
#ifdef BCOLDIST
  int rows_g[krows]; // global index of rows for a work group
  if (rank_g == 0) {
    for (i=0; i<krows; i++) {
      rows_g[i] = (int) random_at_mostL( (long) nrows);
    }
  }
  MPI_Bcast(&rows_g, krows, MPI_INT, 0, comm_g);
#endif
  
  int col_lbound, col_ubound;
#ifdef BCOLDIST
  bin_range_1D(rank_g, ncols, nprocs_g, &col_lbound, &col_ubound);
#else
  col_lbound = 0;
  col_ubound = ncols;
#endif
  
  double t = MPI_Wtime();

  MPI_Win_fence(MPI_MODE_NOPUT | MPI_MODE_NOPRECEDE, win);

  int target_rank, target_disp;
  for (i=0; i<qrows; i++) {

#ifdef BCOLDIST
    int trow = rows_g[i];
#else
    int trow = (int) random_at_mostL( (long) nrows);
#endif
    
    target_rank = bin_coord_1D(trow, nrows, nprocs);
    target_disp = bin_index_1D(trow, nrows, nprocs) * ncols + col_lbound;
    MPI_Get( &B[i*qcols], qcols, MPI_DOUBLE, target_rank, target_disp, qcols, MPI_DOUBLE, win);
  }

  MPI_Win_fence(MPI_MODE_NOSUCCEED, win);

  double tcomm = MPI_Wtime() - t;
  if (rank == 0) {
    printf("Comm time: %f (s)\n", tcomm);
  }

  MPI_Win_free(&win);

#ifdef MPIALLOC
  MPI_Free_mem(A);
#else
  free(A);
#endif

  /* do work on B here */

#ifdef MPIALLOC
  MPI_Free_mem(B);
#else
  free(B);
#endif

  MPI_Finalize();
  return 0;
}
