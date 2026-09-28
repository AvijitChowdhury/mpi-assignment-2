#!/bin/bash
#SBATCH --job-name=heat_omp
#SBATCH --partition=short
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=40
#SBATCH --time=02:00:00
#SBATCH --output=heat_%j.out

##############################################################
# NOTE: Submit from inside ~/heating directory:
#   cd ~/heating
#   sbatch scripts/testjob.sh
# Output will appear as heat_JOBID.out in that directory
##############################################################

module load gcc/64/14.2.0

cd "$SLURM_SUBMIT_DIR"

echo "=========================================="
echo " 159.735 Assignment 3 - Heat Distribution"
echo " Job ID    : $SLURM_JOB_ID"
echo " Node      : $SLURM_NODELIST"
echo " Submit dir: $SLURM_SUBMIT_DIR"
echo " Working   : $(pwd)"
echo "=========================================="

# Verify executables
if [ ! -f "./heat_omp" ] || [ ! -f "./heat_seq" ]; then
    echo "ERROR: Executables not found. Running make..."
    make all
fi

# ============================================================
# SEQUENTIAL BASELINE
# ============================================================
echo ""
echo "=== Sequential Baseline (npix=500) ==="
./heat_seq 500

# ============================================================
# AMDAHL'S LAW: Fixed problem size (npix=500), vary threads
# ============================================================
echo ""
echo "=== Amdahl's Law (fixed npix=500) ==="
for NT in 1 2 4 8 12 16 20 24 28 32 36 40; do
    echo ""
    echo "--- Amdahl: threads=$NT, npix=500 ---"
    OMP_NUM_THREADS=$NT ./heat_omp 500 $NT
done

# ============================================================
# GUSTAFSON'S LAW: Scale problem with thread count
# Per-thread area kept constant: npix = round(500 * sqrt(NT))
# ============================================================
echo ""
echo "=== Gustafson's Law (scaled problem size) ==="

NT=1;  NPIX=500;  echo ""; echo "--- Gustafson: threads=$NT, npix=$NPIX ---"; OMP_NUM_THREADS=$NT ./heat_omp $NPIX $NT
NT=2;  NPIX=707;  echo ""; echo "--- Gustafson: threads=$NT, npix=$NPIX ---"; OMP_NUM_THREADS=$NT ./heat_omp $NPIX $NT
NT=4;  NPIX=1000; echo ""; echo "--- Gustafson: threads=$NT, npix=$NPIX ---"; OMP_NUM_THREADS=$NT ./heat_omp $NPIX $NT
NT=6;  NPIX=1225; echo ""; echo "--- Gustafson: threads=$NT, npix=$NPIX ---"; OMP_NUM_THREADS=$NT ./heat_omp $NPIX $NT
NT=8;  NPIX=1414; echo ""; echo "--- Gustafson: threads=$NT, npix=$NPIX ---"; OMP_NUM_THREADS=$NT ./heat_omp $NPIX $NT
NT=10; NPIX=1581; echo ""; echo "--- Gustafson: threads=$NT, npix=$NPIX ---"; OMP_NUM_THREADS=$NT ./heat_omp $NPIX $NT
NT=12; NPIX=1732; echo ""; echo "--- Gustafson: threads=$NT, npix=$NPIX ---"; OMP_NUM_THREADS=$NT ./heat_omp $NPIX $NT
NT=16; NPIX=2000; echo ""; echo "--- Gustafson: threads=$NT, npix=$NPIX ---"; OMP_NUM_THREADS=$NT ./heat_omp $NPIX $NT
NT=20; NPIX=2236; echo ""; echo "--- Gustafson: threads=$NT, npix=$NPIX ---"; OMP_NUM_THREADS=$NT ./heat_omp $NPIX $NT

echo ""
echo "=========================================="
echo " All runs complete"
echo "=========================================="
