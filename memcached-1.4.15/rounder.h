#ifndef ROUNDER_HPP
#define ROUNDER_HPP

#include <pthread.h>
#include "common.hpp"
#include "statistics_algorithm_base.hpp"

#define act_t int

#define BucketIndex(i) ( (T+i) % (B+1) )
#define TailBucketIndex ( T % (B+1) )
#define HeadBucketIndex ( (T+B-1) % (B+1) )


class rounder : public statistics_algorithm_base {
public: 
  act_t B;
  act_t T;
  int *BUCKETS;
  int perbucket;
  pthread_mutex_t st_lock;
  pthread_mutex_t age_lock;
  rounder() : B(0), T(0) {}
  rounder(programoptions p) :  statistics_algorithm_base(p) {
    B = p.statistics_buckets;
    T = 1;
    pthread_mutex_init(&st_lock, NULL);
    pthread_mutex_init(&age_lock, NULL);
    perbucket = p.cachesize / B;
    fprintf(stderr, "Initialized statistics:\n"
        "(cachesize, # buckets, perbucket )=(%d,%d,%d)\n",
        p.cachesize, B , perbucket);
    BUCKETS = (int *) calloc(B + 1, sizeof(int));
  }

  using statistics_algorithm_base::update_pdf;
  using statistics_algorithm_base::make_cdf;
  virtual void Miss(const KeyType key);
  virtual void Hit(item *it, int dist=-1);
  virtual void Set(item *it);
  virtual void Evict(item *it);
  virtual void printStatistics();

  void print_buckets();
  inline int BucketVal(int index) { return BUCKETS[BucketIndex(index)]; }
  inline int TailBucketVal() { return BUCKETS[TailBucketIndex]; }
  inline int HeadBucketVal() { return BUCKETS[HeadBucketIndex]; }

  int bucketsum();
  void age_if_full();
  void statistics_sanitycheck();
  interval_t rounder_stackdistance(item *findme);
  
  // undo delete, required for memcached, not tested in syslab_cache
//  void set_nodel(item *it);
};

#endif // ROUNDER_HPP
