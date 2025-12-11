/* squasher.c: Sample implementation for performing fast random
               assignment of measurement outcome to timestamp events
               with multiple measurement outcomes (squashing).

Copyright (C) 2025, Justin Peh, Centre for Quantum Technologies
                    <pyuxiang@u.nus.edu>

Due to detector noise or other reasons, the probability of measuring
multi-channel timestamp events is non-zero. Some QKD security proofs
consider information gain from multi-photon emissions by Eve [1,2],
which can be removed by applying a squashing operation [3], effectively
mapping multi-click events to a single click event. This is implemented
here as proof-of-correctness for subsequent incorporation in 'chopper.c'
and 'chopper2.c'. Fast bit manipulation algorithms are credited to [4].

Basic idea: for a multi-click event, say b1101, we want to randomly
assign one of the click events to generate only single photon events,
say b0100. This requires us to poll random numbers from the space of size
N = {2, 3, 4} bit-widths, corresponding to the total number of coincident
clicks. We use the LFSR PRNG previously packaged in 'rnd.c'.

For each N bit-width of samples we want to generate, we choose a suitable
value of m and the largest possible k(N), such that N^k <= 2^m, and the
acceptance ratio R(N) = N^k/2^m to be as large as possible. For example,
for m = 2, we have k(2) = 2 samples per generation, and k(3) = k(4) = 1
(since there are only 2 bits). But while acceptance ratio R(2) = R(4) = 1,
we have R(3) = 75% since we have to discard the b11 outcome to have an
uniform distribution of (three) outcomes {b00, b01, b10}. Constraint being
'PRNG_value()' eats only signed integers, i.e. m < 31. Suitable values are:

  - m(3) = 27, R(3) = 96.2%.
  - m(2) = m(4) = 30, R(2) = R(4) = 100%.

After profiling, the overhead from maintaining a running value is higher
compared to direct generation of PRNG bits per call, for N = {2, 4}, so
this method is only applied to N = 3. Obtained runtime values are as follows,
on AMD 5500U CPU with -O2 flag. In all cases, the runtime is strongly limited
by the LFSR sampling rate. In practice, since multi-events are unlikely,
this should ideally also have negligible average runtime impact.

    +----------+---------------------+
    | N (bits) | Time per event (ns) |
    +----------+---------------------+
    |   0 / 1  |     negligible      |
    |     2    |        5.6          |
    |     3    |       10.2          |
    |     4    |        9.6          |
    +----------+---------------------+

References:
    [1]: N. Lutkenhaus, Phys. Rev. A 59, 3301 (1999)
    [2]: N. Lutkenhaus, Phys. Rev. A 61, 052304 (2000)
    [3]: N.J. Beaudry, et. al., Phys. Rev, Lett. 101, 093601 (2008)
    [4]: S.E. Anderson, <https://graphics.stanford.edu/~seander/bithacks.html> (2011)

Note:
    Compile with: 'gcc -O2 -o squasher squasher.c ../errorcorrection/rnd.c -lm'
*/

#include <stdio.h>
#include <time.h>
#include <math.h>
#include "../errorcorrection/rnd.h"

#define CLEAR_LSB(X)  ((X) & ((X) - 1))  /* unset least-significant set bit */
#define GET_LSB(X)    ((X) & (-(X)))     /* get least-significant set bit */
#define BUFFER_SIZE 27

unsigned int prng_pool, prng_pool_size, prng_pool_size_init, prng_pool_bound;

unsigned int get_random_rank(unsigned int n) {
  switch (n) {
    case 2: return PRNG_value(1);
    case 4: return PRNG_value(2);
  }

  // Fallover for N = 3, refill PRNG values if necessary
  if (prng_pool_size == 0) {
    do {
      prng_pool = PRNG_value(BUFFER_SIZE);
    } while (prng_pool >= prng_pool_bound);
    prng_pool_size = prng_pool_size_init;
  }

  prng_pool_size--;
  unsigned int rank = prng_pool % n; /* zero-indexed */
  prng_pool /= n;
  return rank;
}

long long get_time() {
  struct timespec spec;
  clock_gettime(CLOCK_MONOTONIC, &spec);
  return (long long)spec.tv_sec * 1000000000 + spec.tv_nsec;
}

int main() {
  set_PRNG_seed(time(NULL));
  PRNG_value(40000);  // warm up LFSR

  // Special case for N = 3
  prng_pool = 0;
  prng_pool_size = 0;
  prng_pool_size_init = (unsigned int)floor(log(1 << BUFFER_SIZE) / log(3));
  prng_pool_bound = (unsigned int)pow(3, prng_pool_size_init);

  long long start = get_time(), end;
  unsigned int dpatt, _dpatt, t, n, singlebit, rank; /* _dpatt = dv & 0xf; */
  for (_dpatt = 0; _dpatt < 16; _dpatt++) {
    printf("%x:", _dpatt);
    for (unsigned int i = 0; i < 40; i++) {
      dpatt = _dpatt;

      /* Algorithm start */
      singlebit = CLEAR_LSB(dpatt) == 0; /* check if up to 1 bit set */
      if (!singlebit) {
        for (t = dpatt, n = 0; t; n++) t = CLEAR_LSB(t); /* bit count */
        rank = get_random_rank(n);
        while (rank--) dpatt = CLEAR_LSB(dpatt);
        dpatt = GET_LSB(dpatt);
      }
      /* Algorithm end */

      printf(" %d", dpatt);
    }
    printf("\n");
  }
  end = get_time();
  printf("Time taken: %lf ns\n", (double)(end - start)/40);
  return 0;
}
