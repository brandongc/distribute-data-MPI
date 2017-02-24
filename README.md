# distribute-data-MPI
Redistribute a large data set into random subsets using one-sided MPI

Sampling is done without replacement by default. To sample with replacement compile with
with -DSIMPLESAMPLING.

Redistribute A(N x M) to NGROUPS random subsets of size B(K x M)

A is distributed by rows to all MPI ranks
B is distributed by rows (or optionally columns) among ranks in the group's communicator

Usage:
./memory-estimate Arows Acols Brows Bgroups

Example:

```
> ./memory-estimate 1500000 500000 1200000 5
Total A: 5.4570 TB
Total B: 21.8279 TB
Total data:   27.2848 TB
```

27.2848TB / 55GB / node = 496 Nodes

```
sbatch -N 512 test_medium.sh
```

Output:
```
Total A: 5.4570 TB
Total B: 21.8279 TB
Total:   27.2848 TB
A per rank: 465.66 MB
B per rank: 1.819 GB
Num procs: 12288
B groups: 5

A dimensions: (1500000, 500000)
B dimensions: (1200000, 500000)
Comm time: 15.223760 (s)
```


