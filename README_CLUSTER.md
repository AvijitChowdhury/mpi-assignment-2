# 159.735 Assignment 2 — Quick Start on CTCP Cluster

## IMPORTANT: submit from the project root directory

The job script uses `$SLURM_SUBMIT_DIR` (set automatically by `sbatch`) to find
the executables. This only works if you **submit from the project root** — the same
folder that contains `bucketsort` and `seq_bucketsort` after `make`.

```
~/mpi_bucketsort/         ← run sbatch from HERE
├── bucketsort            ← built by make
├── seq_bucketsort        ← built by make
├── Makefile
├── src/
└── scripts/
    └── testjob.sh
```

## What changed in v8 (this version)

- **Fixed "cluster couldn't allocate" crash**: The old code had the master
  allocate ALL N floats at once (up to 896 MB at np=28). Now each process
  generates its own data locally — memory per rank stays at ~32 MB regardless
  of how many processes run.
- **Fixed "unable to locate executable"**: testjob.sh now uses
  `$SLURM_SUBMIT_DIR` (the directory where you ran `sbatch`) to `cd` to the
  right place, so SLURM always finds the binaries.
- **Separate `.err` log**: errors go to `bucket_sort_JOBID.err` so you can
  spot problems immediately without grepping through the main output.

## Step 1: Upload and enter project
```bash
scp -P 2044 -r mpi_bucketsort 25016760@it096843.massey.ac.nz:~/
ssh 25016760@it096843.massey.ac.nz -p 2044
cd ~/mpi_bucketsort
```

## Step 2: Load module and compile
```bash
module load openmpi4/intel-openapi/64/4.1.8-with-ucx
make
ls -la bucketsort seq_bucketsort     # confirm both files exist
```

## Step 3: Submit job FROM the project root
```bash
# You must be inside ~/mpi_bucketsort when you run sbatch
sbatch scripts/testjob.sh
squeue -u 25016760
```
Output goes to: `bucket_sort_JOBID.out` and `bucket_sort_JOBID.err`

## Step 4: View results
```bash
cat bucket_sort_*.out
```

## Step 5: Generate graphs
```bash
pip install --user matplotlib numpy   # first time only
make plot
```

## Step 6: Download graphs
```bash
scp -P 2044 25016760@it096843.massey.ac.nz:~/mpi_bucketsort/bucket_*.svg .
scp -P 2044 25016760@it096843.massey.ac.nz:~/mpi_bucketsort/bucket_sort_*.out .
```

## Troubleshooting

| Error | Fix |
|-------|-----|
| `unable to locate ./bucketsort` | You did not `make` yet, or submitted from the wrong directory |
| `slurmstepd: error: execve()` | Same as above — run `ls -la` inside the project root to confirm binary exists |
| `sbatch: error: Unable to allocate` | Partition is busy; try `--partition=main` or wait and resubmit |
| Out of memory / allocation failure | Fixed in v8 by distributed data generation |
| Module not found | Run `module avail openmpi` to see what is installed |
