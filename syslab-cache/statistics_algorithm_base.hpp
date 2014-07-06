#ifndef STATISTICS_ALGORITHM_BASE_HPP
#define STATISTICS_ALGORITHM_BASE_HPP

#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include "common.hpp"
#include "item.hpp"

#include "statistics.h"

typedef struct _classstats {
        int stail;
        unsigned int *buckets;
        float *plus;
        //float *ghostplus;
} classstats;

extern classstats *classes;


class statistics_algorithm_base {
public:
  programoptions p;
  int verbose, cachesize, cdfsize, hits, misses, requests;
  bool writecdf;
  double *PLUS = NULL, *CDF = NULL;

  statistics_algorithm_base() : hits(0), misses(0), requests(0) {}

  // TODO: take cdfsize as a parameter
  statistics_algorithm_base(programoptions p) :
      p(p), hits(0), misses(0), requests(0)  {
    CDF = (double *) calloc(sizeof(double), p.cdfsize + 1);
    PLUS = (double *) calloc(sizeof(double), p.cdfsize + 1);
  }

  void resetStatistics() {
    memset(CDF, 0, sizeof(double) * p.cdfsize+1);
    memset(PLUS, 0, sizeof(double) * p.cdfsize+1);
    hits = 0;
    misses = 0;
    requests = 0;
  }

  // if dist >= 0, then the stackdistance of it is dist
  // otherwise the stackdistance is unknown
  virtual void Hit(item *it, int dist) = 0;
  virtual void Miss(const char *key) = 0;
  virtual void Set(item *it) = 0;
  virtual void Evict(item *it) = 0;
  virtual void printStatistics() = 0;

  void update_pdf(interval_t d) {
    // use the functions in statistics.c
  }

  bool make_cdf() {
    // writeme
    fprintf(stderr, "Making CDF from PLUS\n");
    int i;
    if (requests > 0) {
      double sum = 0.0, accum = 0.0, requestcount = (double) requests;
      for (i = 0; i < p.cdfsize; i++) {
        accum += PLUS[i];
        sum += accum;
        CDF[i] = sum / requestcount;
      }
      fprintf(stderr, "Done making the CDF\n");
      return true;
    } else {
      fprintf(stderr, "Could not make the CDF, 0 requests\n");
      return false;
    }
  }

  void write_cdf(const char *filename) {
    // writeme
    int sz, i; // reverse mapped i
    fprintf(stderr, "Writing the CDF to the file: %s\n", filename);
    FILE *fp = Fopen(filename, "w");
    for (i = 0; i < p.cdfsize; i++) {
      sz = 1 + (int) ( p.cachesize * (i / (0.0 + p.cdfsize)) ); // stack distance 0 is cache size 1
      fprintf(fp, "%d %f\n", sz, CDF[i]);
    }
    fclose(fp);
    fprintf(stderr, "Done writing the CDF to a file\n");
  }
};

#endif // STATISTICS_ALGORITHM_BASE_HPP
