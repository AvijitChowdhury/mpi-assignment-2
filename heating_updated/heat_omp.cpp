/******************************************************************************
 * 159.735 Assignment 3
 * Parallel Heat Distribution using OpenMP
 *
 * Solves the 2D Laplace equation (steady-state heat distribution) on a
 * printed circuit plate using an iterative finite-difference method,
 * parallelised with OpenMP.
 *
 * Algorithm:
 *   g(y,x) = 0.25 * (h(y,x-1) + h(y,x+1) + h(y-1,x) + h(y+1,x))
 *
 * Parallelisation strategy:
 *   - Rows are partitioned among threads (horizontal strip decomposition).
 *   - Each thread independently updates its own rows in g from h.
 *   - A barrier after the update step synchronises all threads before
 *     boundary conditions are re-applied.
 *   - omp single (with implicit barrier) applies boundaries once per step.
 *   - Each thread counts locally converged pixels; atomic add accumulates
 *     them into the shared counter.
 *   - omp single (with implicit barrier) checks convergence, increments
 *     iter, resets the counter, and sets the 'done' flag.
 *   - No thread can proceed to the next iteration until 'done' is updated,
 *     guaranteeing clean termination.
 *
 * Usage:
 *   ./heat_omp <npix> [nthreads]
 *
 * Example:
 *   ./heat_omp 500 4
 ******************************************************************************/

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <omp.h>

#include "arrayff.hxx"
#include "draw.hxx"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <npix> [nthreads]\n";
        return 1;
    }

    const int   npix     = std::atoi(argv[1]);
    const int   nthreads = (argc >= 3) ? std::atoi(argv[2])
                                       : omp_get_max_threads();
    const float tol      = 0.00001f;
    const int   npixx    = npix;
    const int   npixy    = npix;
    const int   nrequired = npixx * npixy;
    const int   ITMAX    = 1000000;

    omp_set_num_threads(nthreads);

    std::cout << "========================================\n";
    std::cout << " 159.735 Assignment 3 - OpenMP\n";
    std::cout << "========================================\n";
    std::cout << " Grid size      : " << npix << " x " << npix << "\n";
    std::cout << " Threads        : " << nthreads << "\n";
    std::cout << " Tolerance      : " << tol << "\n";
    std::cout << "========================================\n";

    Array<float, 2> h(npixy, npixx), g(npixy, npixx);

    fix_boundaries2(h);
    fix_boundaries2(g);

    dump_array<float, 2>(h, "plate0.fit");

    int  iter       = 0;
    int  nconverged = 0;
    bool done       = false;

    double t_start = omp_get_wtime();

    // ---------------------------------------------------------------
    // Main parallel region
    // All variables shared unless declared local inside the region.
    // ---------------------------------------------------------------
    #pragma omp parallel shared(h, g, nconverged, iter, done)
    {
        const int tid   = omp_get_thread_num();
        const int nthds = omp_get_num_threads();

        // Partition interior rows (1 .. npixy-2) among threads
        const int interior_rows   = npixy - 2;
        const int rows_per_thread = (interior_rows + nthds - 1) / nthds;
        const int y_start = 1 + tid * rows_per_thread;
        const int y_end   = std::min(y_start + rows_per_thread, npixy - 1);

        while (!done) {

            // ---- Step 1: Update g from h (each thread its own rows) ----
            for (int y = y_start; y < y_end; ++y) {
                for (int x = 1; x < npixx - 1; ++x) {
                    g(y, x) = 0.25f * (h(y, x-1) + h(y, x+1) +
                                       h(y-1, x)  + h(y+1, x));
                }
            }

            // All threads must finish writing g before boundaries are applied
            #pragma omp barrier

            // ---- Step 2: Re-apply boundaries (one thread only) ----
            //   omp single has an implicit barrier at its end, so all
            //   threads wait here until boundaries are fully updated.
            #pragma omp single
            {
                fix_boundaries2(g);
            }

            // ---- Step 3: Count converged pixels in this thread's rows
            //              and copy g -> h ----
            int local_conv = 0;
            for (int y = y_start; y < y_end; ++y) {
                for (int x = 1; x < npixx - 1; ++x) {
                    if (std::fabs(g(y, x) - h(y, x)) < tol)
                        ++local_conv;
                    h(y, x) = g(y, x);
                }
            }

            // Boundary pixels are fixed; count them as converged
            // so the total can reach nrequired.
            // Handled by the master block below — boundary pixels
            // don't change so they always satisfy the tolerance.
            // We simply add them in the single block.

            // Atomically add this thread's local count to the shared total
            #pragma omp atomic
            nconverged += local_conv;

            // Wait for all threads to finish their local counts
            #pragma omp barrier

            // ---- Step 4: Check convergence and update control state ----
            //   omp single has an implicit barrier — no thread leaves
            //   until done/nconverged are updated for the next iteration.
            #pragma omp single
            {
                // Add boundary pixel contributions (they never move)
                // top + bottom rows: npixx each; left + right cols: (npixy-2) each
                int boundary_pixels = 2 * npixx + 2 * (npixy - 2);
                int total_converged = nconverged + boundary_pixels;

                ++iter;
                if (total_converged >= nrequired || iter >= ITMAX)
                    done = true;

                // Reset for next iteration
                nconverged = 0;
            }
            // implicit barrier — all threads see updated 'done' before looping

        } // while !done
    } // omp parallel

    double elapsed = omp_get_wtime() - t_start;

    dump_array<float, 2>(h, "plate1.fit");

    std::cout << "\n========================================\n";
    std::cout << " Results\n";
    std::cout << "========================================\n";
    std::cout << " Iterations   : " << iter    << "\n";
    std::cout << " Threads      : " << nthreads << "\n";
    std::cout << " Elapsed time : " << elapsed << " s\n";
    std::cout << " Throughput   : "
              << (double)iter / elapsed << " iter/s\n";
    std::cout << "========================================\n";

    return 0;
}
