# =============================================================================
# Makefile for 159.735 Assignment 3 — Heat Distribution
# Targets:
#   make all        build heat_demo, heat_seq, heat_omp
#   make heat_demo  build demo only
#   make heat_seq   build sequential solver
#   make heat_omp   build OpenMP parallel solver
#   make clean      remove binaries and object files
#
# CTCP CLUSTER:
#   Before running make, load the GCC module:
#       module load gcc/64/14.2.0
#   Install cfitsio locally first (see INSTRUCTIONS.md), then verify
#   that the LIBP and INCP lines below point to your installation.
# =============================================================================

CPP      = g++
CFLAGS   = -w -O3 -std=c++11
OMPFLAGS = -fopenmp

# ----------------------------------------------------------------------
# CFITSIO paths
# On CTCP: set these to your local installation (see INSTRUCTIONS.md)
# Typical path after following the install steps:
# ----------------------------------------------------------------------
LIBP = -L$(HOME)/cfitsio-4.6.2/.libs
INCP = -I$(HOME)/cfitsio-4.6.2

# On hpc-parallelgpu01 or a system with a global install, comment the
# two lines above and uncomment these instead:
# LIBP = -L/usr/lib -L/usr/local/lib
# INCP = -I/usr/include -I/usr/local/include

LIBS = -lcfitsio -lm
OBJS = fitsfile.o
MODS = $(INCP) $(LIBP) $(LIBS) $(OBJS)

BINS = heat_demo heat_seq heat_omp

all: $(BINS)

clean:
	rm -f $(BINS) *.o *.fit

# Shared FITS I/O object (compiled once)
fitsfile.o: fitsfile.cpp fitsfile.h
	$(CPP) $(CFLAGS) $(INCP) -c fitsfile.cpp

# Demo program (provided — just draws the initial board)
heat_demo: heat_demo.cpp draw.hxx array.hxx $(OBJS)
	$(CPP) $(CFLAGS) -o heat_demo heat_demo.cpp $(MODS)

# Sequential solver
heat_seq: heat_seq.cpp draw.hxx array.hxx $(OBJS)
	$(CPP) $(CFLAGS) -o heat_seq heat_seq.cpp $(MODS)

# OpenMP parallel solver
heat_omp: heat_omp.cpp draw.hxx array.hxx $(OBJS)
	$(CPP) $(CFLAGS) $(OMPFLAGS) -o heat_omp heat_omp.cpp $(MODS)
