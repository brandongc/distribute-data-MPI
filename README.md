# distribute-data-MPI
Redistribute a large data set into random subsets using one-sided MPI

Redistribute A(N x M) to NGROUPS random subsets of size B(K x M)

A is distributed by rows to all MPI ranks
each B is distributed by cols among all MPI ranks in the group's communicator
