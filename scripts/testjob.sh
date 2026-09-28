#!/bin/bash
#SBATCH --job-name=bucket_sort
#SBATCH --partition=short
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=28
#SBATCH --cpus-per-task=1
#SBATCH --mem=16G
#SBATCH --time=02:00:00
#SBATCH --output=%x_%j.out
#SBATCH --error=%x_%j.err

# ── Robust working-directory resolution ──────────────────────────────────────
# SLURM_SUBMIT_DIR is set by sbatch to wherever "sbatch ..." was run from.
# This is the most reliable way to find the project root on any cluster.
if [ -n "$SLURM_SUBMIT_DIR" ]; then
    cd "$SLURM_SUBMIT_DIR"
elif [ -f "$(dirname "$(readlink -f "$0")")/../bucketsort" ]; then
    cd "$(dirname "$(readlink -f "$0")")/.."
fi

echo "=========================================="
echo " 159.735 Assignment 2 - Parallel Bucket Sort"
echo " Job ID    : $SLURM_JOB_ID"
echo " Node      : $SLURM_NODELIST"
echo " Tasks     : $SLURM_NTASKS"
echo " Dir       : $(pwd)"
echo " Executable: $(ls -la bucketsort seq_bucketsort 2>&1)"
echo "=========================================="

# ── Sanity-check: ensure executables exist before running ────────────────────
if [ ! -x "./bucketsort" ] || [ ! -x "./seq_bucketsort" ]; then
    echo "ERROR: bucketsort or seq_bucketsort not found in $(pwd)"
    echo "Please run 'make' in the project root before submitting."
    echo "Files present:"
    ls -la
    exit 1
fi

# ── Load MPI module ───────────────────────────────────────────────────────────
module load openmpi4/intel-openapi/64/4.1.8-with-ucx

ELEM_PER_PROC=8000000

echo ""
echo "=== Sequential Baseline (N=${ELEM_PER_PROC}) ==="
./seq_bucketsort ${ELEM_PER_PROC}

echo ""
echo "--- Gustafson Law: elem_per_proc=${ELEM_PER_PROC} fixed ---"

for NP in 1 2 4 7 8 10 14 16 20 28; do
    N=$(( ELEM_PER_PROC * NP ))
    echo ""
    echo "==> np=${NP}, N=${N} (${ELEM_PER_PROC} per proc)"
    mpirun --mca btl ^openib --oversubscribe -np ${NP} ./bucketsort ${N}
done

echo ""
echo "=== ALL RUNS COMPLETE ==="
