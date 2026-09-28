# 159.735 Assignment 3 — CTCP Cluster Instructions

## What is in this package

| File | Purpose |
|---|---|
| `heat_seq.cpp` | Sequential heat solver (complete) |
| `heat_omp.cpp` | OpenMP parallel solver (complete) |
| `heat_demo.cpp` | Original demo — draws the initial board only |
| `makefile` | Builds all three programs |
| `draw.hxx` | Boundary-condition routines (do not modify) |
| `array.hxx` | 2-D array template (do not modify) |
| `arrayff.hxx` | FITS read/write wrapper (do not modify) |
| `fits.hxx` | FITS low-level wrapper (do not modify) |
| `fitsfile.h/.cpp` | FITS file I/O implementation (do not modify) |
| `scripts/testjob.sh` | Slurm batch script for CTCP |

---

## Step 1 — Log in to CTCP

```bash
ssh 12345678@it096843.massey.ac.nz -p 2044
```

Replace `12345678` with your student ID.

---

## Step 2 — Install CFITSIO locally (do this ONCE)

You do NOT need sudo. This installs cfitsio into your home directory.

```bash
cd ~
wget https://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c/cfitsio-4.6.2.tar.gz
tar zxvf cfitsio-4.6.2.tar.gz
cd cfitsio-4.6.2
./configure
make
# Clean up intermediate object files to save space
rm *.o
rm .libs/*.o
cd ~
```

The libraries will be in `~/cfitsio-4.6.2/.libs/` — this is already
what the makefile points to. No further action needed.

---

## Step 3 — Upload and unpack the project

From your **local machine**, upload the zip:

```bash
scp -P 2044 heating_final.zip 12345678@it096843.massey.ac.nz:~
```

Then on CTCP:

```bash
cd ~
unzip heating_final.zip
mv heating_final heating      # rename if you like
cd heating
```

---

## Step 4 — Load the compiler module

```bash
module load gcc/64/14.2.0
```

You must do this every time you log in before compiling.  
Add it to `~/.bashrc` to load automatically:

```bash
echo "module load gcc/64/14.2.0" >> ~/.bashrc
```

---

## Step 5 — Build

```bash
cd ~/heating
make all
```

You should see three binaries created: `heat_demo`, `heat_seq`, `heat_omp`.

If you see a linker error about cfitsio not found, double-check that
the library was built in Step 2 and that `LIBP`/`INCP` in the makefile
point to the correct path.

---

## Step 6 — Quick interactive test (small size only)

You can run a small test interactively just to verify the binary works.
**Do not run large jobs directly from the shell.**

```bash
./heat_seq 100
./heat_omp 100 4
```

This should complete in a few seconds and create `plate0.fit` and `plate1.fit`.

---

## Step 7 — Submit the full benchmark as a Slurm job

```bash
sbatch scripts/testjob.sh
```

Note the job number printed (e.g. `Submitted batch job 31059`).

Check status:

```bash
squeue -u 12345678
```

- `R` = running  
- `PD` = pending (waiting for resources)  
- Not listed = finished

Read the output file when done:

```bash
less heat_31059.out     # replace 31059 with your job number
```

---

## Step 8 — View FITS images (on your local machine)

Copy the `.fit` files back to your local machine:

```bash
scp -P 2044 12345678@it096843.massey.ac.nz:~/heating/plate*.fit .
```

Open with ds9:

```bash
ds9 plate0.fit &
ds9 plate1.fit &
```

Use the rainbow colour map and linear stretch (Scale → Linear,
Color → Rainbow) to see the heat distribution.

---

## Troubleshooting

### `module: command not found`
You are not on a CTCP login shell. Make sure you are logged in to
`it096843.massey.ac.nz` (not your local machine).

### `error: cfitsio.h: No such file or directory`
The INCP path in the makefile does not match where cfitsio was installed.
Run `ls ~/cfitsio-4.6.2/` to check the header is there.

### `cannot find -lcfitsio`
The LIBP path does not point to the `.libs` directory.
Run `ls ~/cfitsio-4.6.2/.libs/` and look for `libcfitsio.a`.

### `./heat_omp: error while loading shared libraries: libgomp.so`
The gcc module was not loaded. Run `module load gcc/64/14.2.0` first.

### Job stays in `R` for more than 10 minutes
Cancel it and check the output file for errors:

```bash
scancel <jobid>
less heat_<jobid>.out
```

---

## Expected output (example for npix=500, 4 threads)

```
========================================
 159.735 Assignment 3 - OpenMP
========================================
 Grid size      : 500 x 500
 Threads        : 4
 Tolerance      : 1e-05
========================================
plate0.fit 250000
plate1.fit 250000

========================================
 Results
========================================
 Iterations   : <N>
 Threads      : 4
 Elapsed time : <T> s
 Throughput   : <iter/s>
========================================
```

The sequential and OMP versions should converge in the same number of
iterations (they implement exactly the same algorithm and convergence
criterion).
