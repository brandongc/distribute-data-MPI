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

