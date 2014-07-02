#ifndef CACHE_ALGORITHM_BASE_HPP
#define CACHE_ALGORITHM_BASE_HPP

#include <mutex>
#include <atomic>
#include "common.hpp"
#include "statistics_algorithm_base.hpp"
#include "hashmap.hpp"

using namespace std;


class cache_algorithm_base {
public:
  programoptions p;
  atomic_int currentsize;
  hashmap hm;

  statistics_algorithm_base *statistics;
  cache_algorithm_base() {}
  cache_algorithm_base(int cachesize) :  currentsize(0) {
    p.cachesize = cachesize;
  }

  cache_algorithm_base(programoptions p) : p(p), currentsize(0) {
  }

  virtual void set(const char *key, const char *val) = 0;
  virtual item* get(const char *key) = 0;
  virtual int get_stackdistance(item *findme) = 0;
  virtual void printCacheContents() = 0;
  virtual ~cache_algorithm_base() {
    printf("Destroying cache_algorithm_base\n");
  }
};

#endif // CACHE_ALGORITHM_BASE_HPP
