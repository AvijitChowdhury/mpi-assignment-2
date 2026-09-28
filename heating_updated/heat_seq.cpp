/******************************************************************************
 * 159.735 Assignment 3
 * Sequential Heat Distribution Solver
 *
 * Solves the 2D steady-state heat equation (Laplace's equation) on a
 * printed circuit plate using iterative finite-difference relaxation.
 *
 *   g(y,x) = 0.25 * (h(y,x-1) + h(y,x+1) + h(y-1,x) + h(y+1,x))
 *
 * Iteration continues until all interior pixels change by less than tol,
 * or ITMAX iterations are reached.
 *
 * Usage:
 *   ./heat_seq <npix>
 *
 * Example:
 *   ./heat_seq 500
 ******************************************************************************/

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>

#include "arrayff.hxx"
#include "draw.hxx"

static double get_time()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <npix>\n";
        return 1;
    }

    const int   npix      = std::atoi(argv[1]);
    const int   npixx     = npix;
    const int   npixy     = npix;
    const int   nrequired = npixx * npixy;   // all pixels must converge
    const float tol       = 0.00001f;
    const int   ITMAX     = 1000000;

    std::cout << "========================================\n";
    std::cout << " 159.735 Assignment 3 - Sequential\n";
    std::cout << "========================================\n";
    std::cout << " Grid size  : " << npix << " x " << npix << "\n";
    std::cout << " Tolerance  : " << tol  << "\n";
    std::cout << "========================================\n";

    // h = current plate, g = next-step plate
    Array<float, 2> h(npixy, npixx), g(npixy, npixx);

    fix_boundaries2(h);
    fix_boundaries2(g);

    // Save initial image (before any heat diffusion)
    dump_array<float, 2>(h, "plate0.fit");

    int  iter       = 0;
    int  nconverged = 0;

    double t_start = get_time();

    do {
        // ---- Update: compute g from h ----
        for (int y = 1; y < npixy - 1; ++y) {
            for (int x = 1; x < npixx - 1; ++x) {
                g(y, x) = 0.25f * (h(y, x-1) + h(y, x+1) +
                                    h(y-1, x)  + h(y+1, x));
            }
        }

        // Re-apply fixed boundary conditions to g
        fix_boundaries2(g);

        // ---- Convergence check and copy g -> h ----
        nconverged = 0;
        for (int y = 0; y < npixy; ++y) {
            for (int x = 0; x < npixx; ++x) {
                if (std::fabs(g(y, x) - h(y, x)) < tol)
                    ++nconverged;
                h(y, x) = g(y, x);
            }
        }

        ++iter;

    } while (nconverged < nrequired && iter < ITMAX);

    double elapsed = get_time() - t_start;

    // Save final plate image
    dump_array<float, 2>(h, "plate1.fit");

    std::cout << "\n========================================\n";
    std::cout << " Results\n";
    std::cout << "========================================\n";
    std::cout << " Iterations : " << iter    << "\n";
    std::cout << " Elapsed    : " << elapsed << " s\n";
    std::cout << " Throughput : " << (double)iter / elapsed << " iter/s\n";
    std::cout << "========================================\n";
    std::cout << "Use this elapsed time as T_seq for Amdahl's Law.\n\n";

    return 0;
}
