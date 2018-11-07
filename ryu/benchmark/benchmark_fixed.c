// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

#include <math.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__linux__)
#include <sys/types.h>
#include <unistd.h>
#endif

#include "ryu/ryu2.h"
#include "third_party/mersenne/random.h"

static int BUFFER_SIZE = 2000;

static double int64Bits2Double(uint64_t bits) {
  double f;
  memcpy(&f, &bits, sizeof(double));
  return f;
}

struct mean_and_variance {
  int64_t n;
  double mean;
  double m2;
};

typedef struct mean_and_variance mean_and_variance;

  void init(mean_and_variance* mv) {
    mv->n = 0;
    mv->mean = 0;
    mv->m2 = 0;
  }

  void update(mean_and_variance* mv, double x) {
    ++mv->n;
    double d = x - mv->mean;
    mv->mean += d / mv->n;
    double d2 = x - mv->mean;
    mv->m2 += d * d2;
  }

  double variance(mean_and_variance* mv) {
    return mv->m2 / (mv->n - 1);
  }

  double stddev(mean_and_variance* mv) {
    return sqrt(variance(mv));
  }

double generate_double(uint64_t* r) {
  *r = RandomU64();
  double f = int64Bits2Double(*r);
  return f;
}

static int bench64(const uint32_t samples, const uint32_t iterations, const int32_t precision, const bool verbose) {
  char bufferown[BUFFER_SIZE];
  char buffer[BUFFER_SIZE];
  char fmt[100];
  snprintf(fmt, 100, "%%.%df", precision);

  RandomInit(12345);
  mean_and_variance mv1;
  init(&mv1);
  mean_and_variance mv2;
  init(&mv2);
  int throwaway = 0;
  for (int i = 0; i < samples; ++i) {
    uint64_t r = 0;
    const double f = generate_double(&r);

//    printf("%f\n", f);
    clock_t t1 = clock();
    for (int j = 0; j < iterations; ++j) {
      d2fixed_buffered(f, precision, bufferown);
      throwaway += bufferown[2];
    }
    clock_t t2 = clock();
    double delta1 = (t2 - t1) / (double) iterations / CLOCKS_PER_SEC * 1000000000.0;
    update(&mv1, delta1);

    double delta2 = 0.0;
    t1 = clock();
    for (int j = 0; j < iterations; ++j) {
      snprintf(buffer, BUFFER_SIZE, fmt, f);
      throwaway += buffer[2];
    }
    t2 = clock();
    delta2 = (t2 - t1) / (double) iterations / CLOCKS_PER_SEC * 1000000000.0;
    update(&mv2, delta2);

    if (verbose) {
      printf("%s,%" PRIu64 ",%f,%f\n", bufferown, r, delta1, delta2);
    }

//    printf("For %16" PRIX64 " %28s %28s\n", r, bufferown, buffer);
    if (strcmp(bufferown, buffer) != 0) {
      printf("For %16" PRIX64 " %28s %28s\n", r, bufferown, buffer);
    }
  }
  if (!verbose) {
    printf("64: %8.3f %8.3f", mv1.mean, stddev(&mv1));
    printf("     %8.3f %8.3f", mv2.mean, stddev(&mv2));
    printf("\n");
  }
  return throwaway;
}

int main(int argc, char** argv) {
  if (false) {
    double d = int64Bits2Double(0xC2865737987A884E);
    printf("%s\n", d2fixed(d, 9));
    return 0;
  }
  int32_t samples = 1000;
  int32_t iterations = 1000;
  int32_t precision = 800;
  bool verbose = false;
  for (int i = 1; i < argc; i++) {
    char* arg = argv[i];
    if (strcmp(arg, "-v") == 0) {
      verbose = true;
    } else if (strncmp(arg, "-samples=", 9) == 0) {
      sscanf(arg, "-samples=%i", &samples);
    } else if (strncmp(arg, "-iterations=", 12) == 0) {
      sscanf(arg, "-iterations=%i", &iterations);
    } else if (strncmp(arg, "-precision=", 11) == 0) {
      sscanf(arg, "-precision=%i", &precision);
    }
  }

  if (!verbose) {
    // No need to buffer the output if we're just going to print three lines.
    setbuf(stdout, NULL);
  }

  if (verbose) {
    printf("%sryu_time_in_ns%s\n", "ryu_output,float_bits_as_int,", ",grisu3_time_in_ns");
  } else {
    printf("    Average & Stddev Ryu%s\n", "  Average & Stddev Grisu3");
  }
  int throwaway = 0;
  throwaway += bench64(samples, iterations, precision, verbose);
  if (argc == 1000) {
    // Prevent the compiler from optimizing the code away.
    printf("%d\n", throwaway);
  }
  return 0;
}
