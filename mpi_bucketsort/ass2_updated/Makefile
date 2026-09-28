# 159.735 Assignment 2 — Makefile
# Usage:
#   make          build bucketsort and seq_bucketsort
#   make plot     generate SVG graphs from latest .out file
#   make clean    remove binaries

CXX      = mpic++
CXXFLAGS = -O3 -std=c++11 -Wall
SEQCXX   = g++
SEQFLAGS = -O3 -std=c++11 -Wall

.PHONY: all clean plot

all: bucketsort seq_bucketsort

bucketsort: src/bucketsort.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

seq_bucketsort: src/seq_bucketsort.cpp
	$(SEQCXX) $(SEQFLAGS) -o $@ $<

plot:
	@FILE=$$(ls -t bucket_sort_*.out bucket_*.out 2>/dev/null | head -1); \
	if [ -z "$$FILE" ]; then echo "No .out file found. Run the job first."; exit 1; fi; \
	echo "Plotting $$FILE ..."; \
	python3 scripts/plot_results.py $$FILE

clean:
	rm -f bucketsort seq_bucketsort bucket_*.svg bucket_summary.txt
