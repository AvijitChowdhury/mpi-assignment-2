/**
 * 159.735 Assignment 2 — Sequential Bucket Sort (no MPI)
 * Linux compatible. Gustafson baseline: N = elements_per_proc (for np=1).
 * Usage: ./seq_bucketsort [N]
 */

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <vector>

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

typedef uint64_t U64;
const U64 LCG_A = 1664525ULL;
const U64 LCG_C = 1013904223ULL;
const U64 LCG_M = 4294967296ULL;
static U64 lcg_state = 123456789ULL;

inline float rand_float() {
    lcg_state = (LCG_A * lcg_state + LCG_C) % LCG_M;
    return (float)((double)lcg_state / (double)LCG_M);
}

int main(int argc, char* argv[]) {
    int N = 8000000;
    if (argc >= 2) N = std::atoi(argv[1]);
    int p = 28;   /* match max parallel bucket count */

    std::vector<float> data((size_t)N);
    for (int i = 0; i < N; ++i) data[i] = rand_float();

    double t0 = get_time();

    std::vector<std::vector<float>> buckets((size_t)p);
    for (float v : data) {
        int b = (int)(v * p);
        if (b >= p) b = p-1;
        buckets[(size_t)b].push_back(v);
    }

    std::vector<float> sorted;
    sorted.reserve((size_t)N);
    for (auto& bkt : buckets) {
        std::sort(bkt.begin(), bkt.end());
        for (float v : bkt) sorted.push_back(v);
    }

    double elapsed = get_time() - t0;

    bool ok = true;
    for (int i = 1; i < N; ++i)
        if (sorted[i] < sorted[i-1]) { ok = false; break; }

    printf("\n================================================\n");
    printf(" Sequential Bucket Sort (no MPI)\n");
    printf("================================================\n");
    printf(" N elements    : %d\n", N);
    printf(" Result correct: %s\n", ok ? "YES" : "NO");
    printf(" Elapsed time  : %.6f s\n", elapsed);
    printf("================================================\n\n");
    printf("Gustafson baseline T_s = %.6f s  (for 1 proc)\n\n", elapsed);
    return 0;
}
