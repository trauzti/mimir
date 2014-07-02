#ifndef BASIC_HPP
#define BASIC_HPP

#include <cassert>
#include "basic.hpp"
#include "common.hpp"
#include "statistics_algorithm_base.hpp"

class basic : public statistics_algorithm_base {
public:
  using statistics_algorithm_base::statistics_algorithm_base;
  // if dist >= 0, then the stackdistance of it is dist
  // otherwise the stackdistance is unknown
  void Hit(item *it, int dist) {
    __sync_fetch_and_add(&hits, 1);
    __sync_fetch_and_add(&requests, 1);
    if (dist >=0 && p.writecdf) {
      interval_t d = {dist, dist+1};
      update_pdf(d);
    } 
  }

  void Miss(const char *key) {
    __sync_fetch_and_add(&misses, 1);
    __sync_fetch_and_add(&requests, 1);
  }

  void Set(item *it) {}
  void Evict(item *it) {}
  
  void printStatistics() {
    fprintf(stderr, "hits = %d, misses = %d\n", hits, misses);
    if (requests > 0) {
      assert ( hits + misses == requests );
      fprintf(stderr, "hitratio = %.5f\n", (hits + 0.0) / requests);
    }
  }
};

#endif // BASIC_HPP
