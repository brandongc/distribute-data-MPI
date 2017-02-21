# distribute-data-MPI
Redistribute a large data set into random subsets using one-sided MPI

Redistribute A(N x M) to NGROUPS random subsets of size B(K x M)

A is distributed by rows to all MPI ranks
each B is distributed by cols among all MPI ranks in the group's communicator

Usage:
./memory-estimate Arows Acols Brows Bgroups

Example:

```
> ./memory-estimate 200000 80000 160000 10
Total A: 119.209 GB
Total B: 953.674 GB
Total data: 1.0477 TB
```

1 TB / 50 GB / node = 20 Nodes

```
>sbatch -N 20 test.sh
```

Output

```
Total A: 119.209 GB
Total B: 953.674 GB
Total:   1.0477 TB
A per rank: 254.31 MB
B per rank: 1.987 GB
Num procs: 480
B groups: 10

A dimensions: (200000, 80000)
B dimensions: (160000, 80000)
Comm time: 18.277085 (s)
```

Redistributed data at ~50 GB/s