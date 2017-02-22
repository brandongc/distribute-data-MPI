#include "bins.h"

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
