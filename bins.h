#ifndef BINS_H
#define BINS_H

// range of size N divided in M bins
// return size of bin i
int bin_size_1D(int i, int N, int M);
// range of size N divided in M bins
// returns which bin owns i
int bin_coord_1D(int i, int N, int M);

// range of size N divided in M bins
// returns index of i within the bin
int bin_index_1D(int i, int N, int M);

// range of size N divided in M bins
// compute the start, end index of the range of bin i
void bin_range_1D(int i, int N,int M, int *start, int *end);

#endif
