#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

long random_at_mostL(long max) {
  unsigned long num_bins = (unsigned long) max + 1, num_rand = (unsigned long) RAND_MAX + 1, bin_size = num_rand / num_bins, defect = num_rand % num_bins;
  long x;
  do {
    x = random();
  }
  while (num_rand - defect <= (unsigned long)x);
  return x/bin_size;
}

int bin_size_1D(int i, int N, int M) {
  // range of size N divided in M bins
  // return size of bin i
  int b = N / M;
  return (i < N % M) ? b+1 : b;
}

int bin_coord_1D(int i, int N, int M) {
  // range of size N divided in M bins
  // returns which bin owns i
  int j,k=0;
  int b = N / M;
  for (j=0; j<M; j++) {    
    k += (j < N % M) ? b+1 : b;
    if (i < k) return j;
  }
}

int bin_index_1D(int i, int N, int M) {
  // range of size N divided in M bins
  // returns index of i within the bin
  int j,s,k=0;
  int b = N / M;
  for (j=0; j<M; j++) {
    s = (j < N % M) ? b+1 : b;
    k += s;
    if (i < k) return i-k+s;
  }
}

void bin_range_1D(int i, int N,int M, int *start, int *end) {
  // range of size N divided in M bins
  // compute the start, end index of the range of bin i
  int j,s,k=0;
  int b = N / M;
  for (j=0; j<i+1; j++) {
    s = (j < N % M) ? b+1 : b;
    k += s;
    if (i < k) {
      *start = k-s;
      *end   = k;
    }
  }
}

void pprintI(char* str, int i, MPI_Comm comm) {
  // for debugging output
  int j;
  int rank, nprocs;  
  MPI_Comm_size(comm, &nprocs);
  MPI_Comm_rank(comm, &rank);
  for (j=0; j<nprocs; j++) {
    if (rank == j) {
      printf("%i: %-20s = %i\n", rank, str, i);
      fflush(stdout);
    }
    MPI_Barrier(comm);
  }
}

void pprint_arrayI(char* str, int *arr, int n, MPI_Comm comm) {
  // for debugging output
  int i,j;
  int rank, nprocs;  
  MPI_Comm_size(comm, &nprocs);
  MPI_Comm_rank(comm, &rank);
  for (j=0; j<nprocs; j++) {
    if (rank == j) {
      for (i=0; i<n; i++) {
	printf("%i: %-20s [%i] = %i\n", rank, str, i, arr[i]);
      }
      fflush(stdout);
    }
    MPI_Barrier(comm);
  }  
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
  long size;

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

    long sizeA = (long) nrows * (long) ncols * sizeof(double);
    long sizeB = (long) ngroups * (long) krows * (long) ncols * sizeof(double);

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

#ifdef DEBUG
  pprintI("local_rows", local_rows, MPI_COMM_WORLD);
#endif

  double *A;
  size = (long) local_rows * (long) ncols * sizeof(double);
  
#ifdef DEBUG
  pprintI("bytes A", local_rows, MPI_COMM_WORLD);
#endif  

#ifdef MPIALLOC
  MPI_Alloc_mem(size, MPI_INFO_NULL, &A);  
#else
  A = (double *) malloc(size);
#endif

  memset(A, rank, size);

  MPI_Win win;
  MPI_Win_create(A, size, sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

  int color = bin_coord_1D(rank, nprocs, ngroups);
  MPI_Comm comm_g;
  MPI_Comm_split(MPI_COMM_WORLD, color, rank, &comm_g);

#ifdef DEBUG
  pprintI("color", color, MPI_COMM_WORLD);
#endif
  
  int nprocs_g, rank_g;
  MPI_Comm_size(comm_g, &nprocs_g);
  MPI_Comm_rank(comm_g, &rank_g);
  
  int qcols = bin_size_1D(rank_g, ncols, nprocs_g);
  double *B;
  size = (long) qcols * (long) krows * sizeof(double);
  
#ifdef DEBUG
  pprintI("B cols", qcols, MPI_COMM_WORLD);
  pprintI("bytes B", size, MPI_COMM_WORLD);
#endif
  
#ifdef MPIALLOC
  MPI_Alloc_mem(size, MPI_INFO_NULL, &B);   
#else  
  B = (double *) malloc(size);
#endif
  
  int rows_g[krows]; // global index of rows for a work group
  if (rank_g == 0) {
    for (i=0; i<krows; i++) {
      rows_g[i] = (int) random_at_mostL( (long) nrows);
    }
  }
  MPI_Bcast(&rows_g, krows, MPI_INT, 0, comm_g);
  
#ifdef DEBUG
  pprint_arrayI("B rows", rows_g, krows, MPI_COMM_WORLD);
#endif

  int col_lbound, col_ubound;
  
  bin_range_1D(rank_g, ncols, nprocs_g, &col_lbound, &col_ubound);
  
  double t = MPI_Wtime();

  MPI_Win_fence(MPI_MODE_NOPUT | MPI_MODE_NOPRECEDE, win);

  int target_rank, target_disp;  
  for (i=0; i<krows; i++) {
    target_rank = bin_coord_1D(rows_g[i], nrows, nprocs);
    target_disp = bin_index_1D(rows_g[i], nrows, nprocs) * ncols + col_lbound;

#ifdef DEBUG
    for (j=0; j<nprocs; j++) {
      if (rank == j) {
	printf("%i: %-20s = %i\n", rank, "row", i);
	printf("%i: %-20s = %i\n", rank, "row", rows_g[i]);	
	printf("%i: %-20s = %i\n", rank, "col_off", col_lbound);	
	printf("%i: %-20s = %i\n", rank, "row_t", bin_index_1D(rows_g[i], nrows, nprocs));
	printf("%i: %-20s = %i\n", rank, "target_rank", target_rank);
	printf("%i: %-20s = %i\n", rank, "target_disp", target_disp);
      }
      fflush(stdout);
      MPI_Barrier(MPI_COMM_WORLD);
    }    
#endif    
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
