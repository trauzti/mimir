#ifndef ROUNDER_HPP
#define ROUNDER_HPP

#include <pthread.h>
#include "common.hpp"
#include "statistics.h"
#include "statistics_algorithm_base.hpp"

#define act_t int

class rounder : public statistics_algorithm_base {
public: 
  rounder() {}
  rounder(programoptions p) :  statistics_algorithm_base(p) {
    statistics_init(p.statistics_buckets);
    //int perbucket = p.cachesize / p.statistics_buckets;
  }

  using statistics_algorithm_base::update_pdf;
  using statistics_algorithm_base::make_cdf;
  virtual void Miss(const char *key);
  virtual void Hit(item *it, int dist=-1);
  virtual void Set(item *it);
  virtual void Evict(item *it);
  virtual void printStatistics();

};

#endif // ROUNDER_HPP
