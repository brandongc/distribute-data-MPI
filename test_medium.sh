#!/bin/bash
#SBATCH -p debug
#SBATCH -t 30

nrows=1500000
ncols=500000
krows=1200000
ngroups=5

N=$((SLURM_NNODES * 24))
exe=./distribute-data

srun -n $N -c 2 --cpu_bind=cores --compress=lz4 --bcast=/tmp/$exe $exe $nrows $ncols $krows $ngroups
