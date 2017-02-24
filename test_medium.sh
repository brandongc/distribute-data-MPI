#!/bin/bash
#SBATCH -p debug
#SBATCH -t 5

nrows=1500000
ncols=500000
krows=1200000
ngroups=5

cpn=$SLURM_CPUS_ON_NODE
if [ "$cpn" -eq 48 ] || [ "$cpn" -eq 64 ]; then
    ranks_per_node=$((cpn/2))
    c=2
elif [ "$cpn" -eq 272 ]; then
    ranks_per_node=$((cpn/4))
    c=4
fi
    
N=$((SLURM_NNODES * ranks_per_node))

exe=./distribute-data

srun -n $N -c 2 --cpu_bind=cores --compress=lz4 --bcast=/tmp/$exe $exe $nrows $ncols $krows $ngroups
