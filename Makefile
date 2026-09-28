# 159.735 Assignment 2 Makefile
# make        — build bucketsort and seq_bucketsort
# make plot   — generate SVG graphs from latest .out file
# make clean  — remove binaries and graphs

CXX      = mpic++
CXXFLAGS = -O3 -std=c++11 -Wall

SEQCXX   = g++
SEQFLAGS = -O3 -std=c++11 -Wall

.PHONY: all clean plot

all: bucketsort seq_bucketsort

bucketsort: src/bucketsort.cpp
	$(CXX) $(CXXFLAGS) -o bucketsort src/bucketsort.cpp

seq_bucketsort: src/seq_bucketsort.cpp
	$(SEQCXX) $(SEQFLAGS) -o seq_bucketsort src/seq_bucketsort.cpp

plot:
	@FILE=$$(ls -t bucket_*.out 2>/dev/null | head -1); \
	if [ -z "$$FILE" ]; then echo "No bucket_*.out file found"; exit 1; fi; \
	echo "Plotting $$FILE ..."; \
	python3 scripts/plot_results.py $$FILE

clean:
	rm -f bucketsort seq_bucketsort bucket_*.svg bucket_summary.txt
