#!/bin/bash
#SBATCH --job-name=bucket_sort
#SBATCH --partition=short
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --time=03:00:00
#SBATCH --output=%x_%j.out

##############################################################
# NOTE: Submit from inside ~/mpi_bucketsort directory:
#   cd ~/mpi_bucketsort
#   sbatch scripts/testjob.sh
# Output will appear as bucket_sort_JOBID.out in that directory
##############################################################

module load openmpi4/intel-openapi/64/4.1.8-with-ucx

cd "$SLURM_SUBMIT_DIR"

echo "=========================================="
echo " 159.735 Assignment 2 - Parallel Bucket Sort"
echo " Job ID    : $SLURM_JOB_ID"
echo " Node      : $SLURM_NODELIST"
echo " Submit dir: $SLURM_SUBMIT_DIR"
echo " Working   : $(pwd)"
echo " Executables:"
ls -lh bucketsort seq_bucketsort 2>/dev/null || echo "  ERROR: executables not found!"
echo "=========================================="

# Verify executables
if [ ! -f "./bucketsort" ]; then
    echo "ERROR: bucketsort not found in $(pwd)"
    echo "Run 'make' first, then 'sbatch scripts/testjob.sh'"
    exit 1
fi
if [ ! -f "./seq_bucketsort" ]; then
    echo "ERROR: seq_bucketsort not found in $(pwd)"
    echo "Run 'make' first"
    exit 1
fi

# Gustafson's Law: elements per process is fixed = 8,000,000
# Total N = ELEM_PER_PROC * np  (grows with np)
ELEM_PER_PROC=8000000

echo ""
echo "=== Sequential Baseline (N=${ELEM_PER_PROC}) ==="
./seq_bucketsort ${ELEM_PER_PROC}

echo ""
echo "--- Parallel runs: Gustafson's Law ---"
echo "--- Elements per proc = ${ELEM_PER_PROC} (fixed) ---"

for NP in 1 2 4 8 12 16 20 24 28 32 36 40; do
    N=$(( ELEM_PER_PROC * NP ))
    echo ""
    echo "==> np=${NP}, N=${N} (${ELEM_PER_PROC} per proc)"
    mpirun --mca btl ^openib --oversubscribe -np ${NP} ./bucketsort ${N}
done

echo ""
echo "=== ALL RUNS COMPLETE ==="
