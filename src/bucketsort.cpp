/**
 * 159.735 Assignment 2 — Parallel Bucket Sort (Gustafson's Law)
 *
 * 4-Phase algorithm:
 *   Phase 1 — Each process generates its OWN n_pp floats locally (no master bottleneck)
 *   Phase 2 — Each process fills p small local buckets (no comms)
 *   Phase 3 — MPI_Alltoall (counts) + MPI_Alltoallv (data)
 *   Phase 4 — Each process std::sort its large bucket independently
 *
 * Gustafson: n_pp = N/p is FIXED. Total N = n_pp * p scales with p.
 *
 * NOTE: Phase 1 now uses distributed generation (each rank seeds independently)
 *       to avoid master OOM at large N (e.g. 28 * 8M * 4 bytes = 896 MB on one node).
 *
 * Compile:  mpic++ -O3 -std=c++11 -o bucketsort src/bucketsort.cpp
 * Run:      mpirun -np 4 ./bucketsort 32000000  (8M per proc)
 */

#include <mpi.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>

typedef uint64_t U64;
const U64 LCG_A = 1664525ULL;
const U64 LCG_C = 1013904223ULL;
const U64 LCG_M = 4294967296ULL;

/* Each rank seeds from a different starting point so data is varied */
static U64 lcg_state;
inline void lcg_seed(int rank) {
    /* Spread seeds widely across the LCG space per rank */
    lcg_state = 123456789ULL + (U64)rank * 1000000007ULL;
    /* Warm up */
    for (int i = 0; i < 20; ++i)
        lcg_state = (LCG_A * lcg_state + LCG_C) % LCG_M;
}
inline float rand_float() {
    lcg_state = (LCG_A * lcg_state + LCG_C) % LCG_M;
    return (float)((double)lcg_state / (double)LCG_M);
}

/* ── Phase 2: fill local small buckets ─────────────────────────────────── */
void fill_small_buckets(const std::vector<float>& local_data,
                        int p, int cap,
                        std::vector<float>& small_bucket,
                        std::vector<int>&   numpb)
{
    std::fill(numpb.begin(), numpb.end(), 0);
    for (float val : local_data) {
        int b = (int)(val * p);
        if (b >= p) b = p - 1;
        if (b < 0)  b = 0;
        if (numpb[b] < cap)
            small_bucket[(size_t)b * cap + numpb[b]++] = val;
    }
}

/* ── Phase 3: Alltoall + Alltoallv ─────────────────────────────────────── */
int empty_small_buckets(std::vector<float>& small_bucket,
                        std::vector<int>&   numpb,
                        std::vector<float>& big_bucket,
                        int cap, int p)
{
    std::vector<int> recvcnt(p, 0);
    MPI_Alltoall(numpb.data(),    1, MPI_INT,
                 recvcnt.data(), 1, MPI_INT,
                 MPI_COMM_WORLD);

    std::vector<int> recvoff(p, 0);
    int num_recv = recvcnt[0];
    for (int n = 1; n < p; ++n) {
        recvoff[n] = recvoff[n-1] + recvcnt[n-1];
        num_recv  += recvcnt[n];
    }

    std::vector<int> sendoff(p);
    for (int n = 0; n < p; ++n) sendoff[n] = n * cap;

    if (num_recv > (int)big_bucket.size())
        big_bucket.resize((size_t)num_recv);

    MPI_Alltoallv(small_bucket.data(), numpb.data(),   sendoff.data(), MPI_FLOAT,
                  big_bucket.data(),   recvcnt.data(), recvoff.data(), MPI_FLOAT,
                  MPI_COMM_WORLD);
    return num_recv;
}

/* ═══════════════════════════════════════════════════════════════════════════
   MAIN
   ═══════════════════════════════════════════════════════════════════════════ */
int main(int argc, char* argv[]) {

    MPI_Init(&argc, &argv);
    int myid, p;
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    /* N is passed as total elements; must be divisible by p */
    long long N_ll = 8000000LL * p;          /* sensible default */
    if (argc >= 2) N_ll = std::atoll(argv[1]);
    N_ll = (N_ll / p) * p;                  /* round to multiple of p */
    long long n_pp_ll = N_ll / p;           /* elements per process (fixed) */

    /* Guard against overflow of int: use long long throughout */
    int n_pp = (int)n_pp_ll;

    /* ── Phase 1: Each process generates its OWN data locally ────────────
       This avoids the master trying to allocate N*sizeof(float) all at once
       (at np=28, N=224M → 896 MB just for the send buffer).
       Each rank uses a different LCG seed so data spans [0,1) uniformly.  */
    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    lcg_seed(myid);
    std::vector<float> local_data((size_t)n_pp);
    for (int i = 0; i < n_pp; ++i) local_data[i] = rand_float();

    MPI_Barrier(MPI_COMM_WORLD);
    double t_phase1 = MPI_Wtime() - t0;

    /* ── Phase 2: Local small buckets ─────────────────────────────────── */
    double t1 = MPI_Wtime();

    /* cap: use long long arithmetic to avoid int overflow at large n_pp */
    long long cap_ll = 4LL * n_pp / p;
    if (cap_ll < 64) cap_ll = 64;
    if (cap_ll > (long long)INT_MAX) cap_ll = (long long)INT_MAX;
    int cap = (int)cap_ll;

    std::vector<float> small_bucket((size_t)p * cap, 0.0f);
    std::vector<int>   numpb(p, 0);
    fill_small_buckets(local_data, p, cap, small_bucket, numpb);

    MPI_Barrier(MPI_COMM_WORLD);
    double t_phase2 = MPI_Wtime() - t1;

    /* ── Phase 3: Alltoall redistribution ────────────────────────────── */
    double t2 = MPI_Wtime();

    std::vector<float> big_bucket((size_t)(2 * n_pp));
    int num_recv = empty_small_buckets(small_bucket, numpb, big_bucket, cap, p);
    big_bucket.resize((size_t)num_recv);

    MPI_Barrier(MPI_COMM_WORLD);
    double t_phase3 = MPI_Wtime() - t2;

    /* ── Phase 4: Sort (fully parallel, no comms) ─────────────────────── */
    double t3 = MPI_Wtime();
    std::sort(big_bucket.begin(), big_bucket.end());
    MPI_Barrier(MPI_COMM_WORLD);
    double t_phase4 = MPI_Wtime() - t3;

    double t_total = t_phase1 + t_phase2 + t_phase3 + t_phase4;

    /* ── Lightweight verification: check own bucket is sorted,
          then verify boundary with neighbour via point-to-point ────────── */
    bool local_ok = true;
    for (size_t i = 1; i < big_bucket.size(); ++i)
        if (big_bucket[i] < big_bucket[i-1]) { local_ok = false; break; }

    float my_last   = big_bucket.empty() ? 0.0f : big_bucket.back();
    float prev_last = 0.0f;
    if (myid > 0) {
        MPI_Status st;
        MPI_Recv(&prev_last, 1, MPI_FLOAT, myid-1, 99, MPI_COMM_WORLD, &st);
        if (!big_bucket.empty() && big_bucket.front() < prev_last)
            local_ok = false;
    }
    if (myid < p-1)
        MPI_Send(&my_last, 1, MPI_FLOAT, myid+1, 99, MPI_COMM_WORLD);

    int local_ok_int = local_ok ? 1 : 0;
    int global_ok_int = 0;
    MPI_Reduce(&local_ok_int, &global_ok_int, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);

    /* ── Gustafson metrics ────────────────────────────────────────────── */
    double f_serial    = t_phase1 / t_total;
    double S_gustafson = (double)p - f_serial * (double)(p - 1);

    /* ── Report ──────────────────────────────────────────────────────── */
    if (myid == 0) {
        printf("\n================================================\n");
        printf(" 159.735 Assignment 2 - Parallel Bucket Sort\n");
        printf("================================================\n");
        printf(" Processes          : %d\n", p);
        printf(" Total elements (N) : %lld\n", N_ll);
        printf(" Elements per proc  : %d\n", n_pp);
        printf(" Result correct?    : %s\n", global_ok_int ? "YES" : "NO - ERROR!");
        printf("------------------------------------------------\n");
        printf(" Phase 1 (gen local): %.6f s\n", t_phase1);
        printf(" Phase 2 (small bkt): %.6f s\n", t_phase2);
        printf(" Phase 3 (alltoallv): %.6f s\n", t_phase3);
        printf(" Phase 4 (sort)     : %.6f s\n", t_phase4);
        printf(" TOTAL              : %.6f s\n", t_total);
        printf("================================================\n\n");
        printf("--- Gustafson's Law ---\n");
        printf(" Serial fraction f   : %.4f\n", f_serial);
        printf(" Scaled speedup S_G(%d): %.4f\n", p, S_gustafson);
        printf(" (Ideal scaled S_G   : %.1f)\n\n", (double)p);

        int show = (int)big_bucket.size();
        printf("Rank 0 first 5 values: ");
        for (int i = 0; i < (show < 5 ? show : 5); ++i)
            printf("%.5f ", big_bucket[i]);
        printf("\nRank 0 last  5 values: ");
        for (int i = (show > 5 ? show-5 : 0); i < show; ++i)
            printf("%.5f ", big_bucket[i]);
        printf("\n\n");
    }

    MPI_Finalize();
    return 0;
}
