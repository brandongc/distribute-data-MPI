#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>


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

  long sizeA, sizeB;
    
  if (argc != 5) {
    printf("Usage: %s Arows Acols Brows Bgroups\n", argv[0]);
    return 0;
  } else {
    nrows = atoi(argv[1]);
    ncols = atoi(argv[2]);
    krows = atoi(argv[3]);
    ngroups = atoi(argv[4]);
  }

  sizeA = (long) nrows * (long) ncols * sizeof(double);
  sizeB = (long) ngroups * (long) krows * (long) ncols * sizeof(double);

  printf("Total A: %s\n", readable_fs((double) sizeA, buf));
  printf("Total B: %s\n", readable_fs((double) sizeB, buf));
  printf("Total data: %s\n", readable_fs( (double) (sizeA+sizeB), buf) );
  
  return 0;
}
