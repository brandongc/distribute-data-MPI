#!/bin/bash
#SBATCH -p debug
#SBATCH -t 5

nrows=200000
ncols=80000
krows=160000
ngroups=10

N=$((SLURM_NNODES * 24))
exe=./distribute-data

echo $SLURM_NNODES $N $nrows $ncols $krows $ngroups

srun -n $N -c 2 --cpu_bind=cores --compress=lz4 --bcast=/tmp/$exe $exe $nrows $ncols $krows $ngroups
