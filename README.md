# 159.735 Assignment 2 — Parallel Bucket Sort (MPI)

## Overview

This MPI program implements **parallel bucket sort** of N random
floating-point numbers in \[0,1\) using a 4-phase strategy.

---

## Algorithm: 4 Phases

### Phase 1 — Data Generation & Scatter

The master process generates N random floats using a linear congruential
generator (same LCG as Assignment 1 for consistency).  The array is
then distributed evenly across all p processes using `MPI_Scatter`,
so each process receives n/p numbers.

**Communication cost:**
```
T_comm1 = p * (t_startup + (n/p) * t_data)
```

### Phase 2 — Fill Small Buckets (local, no communication)

Each process independently examines its n/p numbers and places each
value into one of p "small buckets" based on value range:
- bucket j holds values in \[ j/p, (j+1)/p \)

This is purely local computation:
```
T_comp2 = n/p
```

### Phase 3 — Alltoall(v): Move Small → Large Buckets

This is the key communication phase.

1. **`MPI_Alltoall`** — each process sends its small-bucket *counts*
   to all others, so every process knows exactly how much data it will
   receive. Cost: p small integers exchanged.

2. **`MPI_Alltoallv`** — the actual data redistribution.  Process j
   receives all values belonging to large bucket j (range \[j/p,(j+1)/p\))
   from every other process.

After this phase, each process holds all values for exactly one
"large bucket" — ready to sort independently.

**Expected items per small bucket:** n/p²
**Ideal communication time (all processes in parallel):**
```
T_comm3_ideal = (p-1)*t_startup + (n/p²)*t_data
```
**Actual time is bounded:**
```
T_comm3_ideal ≤ T_comm3 ≤ p * T_comm3_ideal
```

### Phase 4 — Sort Large Buckets in Parallel

Each process sorts its large bucket independently using `std::sort`
(introsort, O((n/p) log(n/p))):
```
T_comp4 = (n/p) * log(n/p)
```

All p processes do this simultaneously — true parallelism.

---

## Files

```
mpi_bucketsort/
├── src/
│   ├── bucketsort.cpp      ← Main MPI parallel bucket sort
│   └── seq_bucketsort.cpp  ← Sequential reference (no MPI)
├── scripts/
│   ├── testjob.sh          ← SLURM job for CTCP cluster
│   └── build_and_run.bat   ← Windows one-click build & run
├── Makefile                ← Linux/Mac build
└── README.md               ← This file
```

---

## Windows Setup

> If you already followed the Windows Setup for Assignment 1, skip to Build.
> MS-MPI and MSYS2 are already installed.

1. Install **MS-MPI**: https://learn.microsoft.com/en-us/message-passing-interface/microsoft-mpi
   - Download and install both `msmpisetup.exe` and `msmpisdk.msi`

2. Install **MSYS2**: https://www.msys2.org
   - Open MSYS2 MinGW64 shell and run:
     ```bash
     pacman -Syu
     pacman -S mingw-w64-x86_64-gcc make
     ```
   - Add `C:\msys64\mingw64\bin` to your Windows PATH.

3. **Build & Run** — open a Command Prompt, go to `scripts\`:
   ```cmd
   cd path\to\mpi_bucketsort\scripts
   build_and_run.bat
   ```

### Manual run (after building)

```cmd
REM Sequential reference
seq_bucketsort.exe 1000000

REM Parallel (change -n for different process counts)
mpiexec -n 1 bucketsort.exe 1000000
mpiexec -n 2 bucketsort.exe 1000000
mpiexec -n 4 bucketsort.exe 1000000
mpiexec -n 8 bucketsort.exe 1000000

REM Larger dataset for more meaningful timings
mpiexec -n 4 bucketsort.exe 8000000
```

**Arguments:**
```
bucketsort.exe  [N]
  N  - total number of floats to sort (default: 1,000,000)
      N is automatically rounded down to nearest multiple of p.
```

---

## Linux / Mac

```bash
# Install MPI (Ubuntu/Debian)
sudo apt-get install openmpi-bin libopenmpi-dev

# Build
cd mpi_bucketsort/
make

# Run
make bench              # sequential reference
make run                # 4 processes, N=1M

mpirun -np 8 ./bucketsort 8000000
```

---

## CTCP Cluster (Massey)

```bash
scp -r mpi_bucketsort/ 12345678@it096843.massey.ac.nz:~/
ssh 12345678@it096843.massey.ac.nz

cd mpi_bucketsort/
module load openmpi4/intel-openapi/64/4.1.8-with-ucx
make
sbatch scripts/testjob.sh
squeue -u 12345678
less slurm-<JOBID>.out
```

---

## Amdahl's Law Analysis

Collect the **Total** time from output for each process count:

| p  | T_parallel (s) | Speedup S = T(1)/T(p) |
|----|----------------|------------------------|
| 1  | ?              | 1.00                   |
| 2  | ?              | ?                      |
| 4  | ?              | ?                      |
| 8  | ?              | ?                      |

Then fit Amdahl's Law:
```
S(p) = 1 / (f + (1-f)/p)
```

The serial fraction f comes mainly from:
- Phase 1: master generating and scattering data (only master works)
- The Alltoall communication in Phase 3 (limited by network bandwidth)

For large N and uniform data, f is small (< 0.05) and you should see
near-linear speedup up to 4–8 processes.

---

## Verification

After gathering all large buckets back to master, the program checks
that the combined array is globally sorted and prints:
```
Result correct? : YES
```

It also prints the first and last 10 values as a spot-check.

---

## Key MPI Functions Used

| Function          | Purpose                                   |
|-------------------|-------------------------------------------|
| `MPI_Scatter`     | Distribute input data from master         |
| `MPI_Alltoall`    | Exchange bucket *counts* (Phase 3 step 1) |
| `MPI_Alltoallv`   | Exchange bucket *data*  (Phase 3 step 2)  |
| `MPI_Barrier`     | Synchronise for accurate timing           |
| `MPI_Gather`      | Collect sorted buckets for verification   |
| `MPI_Gatherv`     | Variable-length gather of sorted data     |
