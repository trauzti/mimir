#include <cstdio>
#include <cmath>
#include <cassert>
#include <cstring>

#include "item.hpp"
#include "rounder.hpp"
#include "common.hpp"

#define DO_SANITYCHECKS 1

/******* Implementation details: ********/
/* T+B-1 is the top bucket
 * T is the bottom bucket
 * B is the number of buckets

  We age elements at stackdistance between 0 and the average stack distance that is hit
  Because of that the elements are not evenly distributed amongst the bins
  However, if we age everything the bins are all used
*/

/* Think: Where do we need to use the age_lock */
/* Think: Do we need to lock the BUCKETS array ? */


void rounder::print_buckets() {
  fprintf(stderr, "BUCKETS=[");
  act_t i;
  int count = 0, val;
  for (i = 0; i < B ; ++i) {
    val = BUCKETS[BucketIndex(i)];
    assert (val >= 0);
    fprintf(stderr, "%d, ", val);
    count += val;
  }
  fprintf(stderr, "],total=%d\n", count);
  assert(count <= p.cachesize);
}


// post: the real stack distance of n is in [d->start,...,d->end-1]
interval_t rounder::rounder_stackdistance(item *it) {
  interval_t d;
  act_t i;
  d.start = 0;
  act_t a = it->activity;

  if (a < T) {
    a = T;
  }
  a -= T;

  for (i = B - 1; i >= 0 ; --i) {
    if (i == a) { // go down until we hit the correct region
      break;
    }
    d.start += BUCKETS[BucketIndex(i)];
  }
  assert (i >= 0);
  assert (i <= B -1 );

  d.end = d.start + BUCKETS[BucketIndex(i)];
  if (d.end < 0 || d.end > p.cachesize) {
    print_buckets();
    printf("rounder_stackdistance(%p->%d)=[%d,%d[\n", it, it->key, d.start, d.end);
  }
  assert ( 0 <= d.start && d.start <= p.cachesize );
  assert ( 0 <= d.end && d.end <= p.cachesize ); // how can this fail in multiple threads ???
  assert ( d.start < d.end );
  return d;
}

int rounder::bucketsum() {
  int i, j, sm = 0;
  for (i = 0; i < B; i++) {
    j = BUCKETS[BucketIndex(i)];
    if (j < 0) {
      print_buckets();
    }
    assert (j >= 0);
    sm += j;
  }
  return sm;
}

void rounder::age_if_full() {
  // Make space for new item
  if (HeadBucketVal()  <  perbucket) {
    return;
  }
  if (0 != pthread_mutex_trylock(&age_lock)) {
    // We can only do aging in one thread
    return;
  }
  statistics_sanitycheck();
  if (p.verbose) {
    fprintf(stderr, "Performing aging\n");
  }
  int smbefore, smafter;
#ifdef DO_SANITYCHECKS
  smbefore = bucketsum();
#endif
  BUCKETS[ (T + B) % (B + 1)] = 0; // New top bin to zero
  T++;
  assert (BUCKETS[HeadBucketIndex] == 0);
  BUCKETS[ T % (B + 1)] += BUCKETS[(T-1) % (B+1)];
  //__sync_fetch_and_add(&BUCKETS[TailBucketIndex], BUCKETS[(T - 1) % (B + 1)]); // Merge bottom bin with old bottom bin
  if (p.verbose) {
    fprintf(stderr, "Performed aging, range is now [%d,%d]\n", T, T + B - 1);
  }
#ifdef DO_SANITYCHECKS
  smafter = bucketsum();
  assert (smbefore == smafter);
#endif
  statistics_sanitycheck();
  pthread_mutex_unlock(&age_lock);
}

void rounder::Hit(item *it, int dist /* =-1 */) {
  pthread_mutex_lock(&st_lock);
  int smbefore, smafter;
  if (p.verbose) {
    printf("Statistics Hit(%d), act=%d\n", it->key, it->activity);
  }
  __sync_fetch_and_add(&hits, 1);
  __sync_fetch_and_add(&requests, 1);
#ifdef DO_SANITYCHECKS
  smbefore = bucketsum();
#endif
  assert (it->activity > 0);
  if (it->activity > 0) {
    interval_t d;
    if (it->activity < T)
      it->activity = T;
    d = rounder_stackdistance(it);
    update_pdf(d);
    __sync_fetch_and_sub(&BUCKETS[it->activity % (B + 1)], 1);
    age_if_full();
    it->activity = T + B - 1;
    __sync_fetch_and_add(&BUCKETS[it->activity % (B + 1)], 1);
    statistics_sanitycheck();
  }
#ifdef DO_SANITYCHECKS
  smafter = bucketsum();
  assert (smbefore == smafter);
#endif
  pthread_mutex_unlock(&st_lock);
}


// set in the cache
void rounder::Set(item *it) {
  pthread_mutex_lock(&st_lock);
  int smbefore, smafter;
  if (p.verbose) {
    printf("Statistics Set(%d), act=%d\n", it->key, it->activity);
  }
#ifdef DO_SANITYCHECKS
  smbefore = bucketsum();
#endif
  bool incache = false;
  if (it->activity != 0) {
    incache = true;
    // calling set on an element already in the cache
    if (it->activity < T) it->activity = T;
    __sync_fetch_and_sub(&BUCKETS[it->activity % (B + 1)], 1);
  }
  // Age other elements (if the first bin is full) before putting this element in the first bin
  age_if_full();
  it->activity = T + B - 1; // Top val
  __sync_fetch_and_add(&BUCKETS[it->activity % (B + 1)], 1);// what if T changes in the meantime ...
  statistics_sanitycheck();
#ifdef DO_SANITYCHECKS
  smafter = bucketsum();
  if (incache) {
    assert (smbefore == smafter);
  } else {
    assert (smbefore + 1 == smafter);
  }
#endif
  pthread_mutex_unlock(&st_lock);
  //printf("endofSet(%d), act=%d\n", it->key, it->activity);
}

// this item was evicted from the main cache
void rounder::Evict(item *it) {
  pthread_mutex_lock(&st_lock);
  if (p.verbose) {
    printf("Statistics Evict(%d), act=%d\n", it->key, it->activity);
  }
  int smbefore, smafter;
  smbefore = bucketsum();
  if (it->activity < T) it->activity = T;
  if (p.verbose) {
    //printf("Evict(%d), in bucket=%d\n", it->key, it->activity % (B+1));
  }
  statistics_sanitycheck();
  act_t a = it->activity;
  assert (a > 0);
  /*
  while (1) {
    savedT = T;
    if (a < savedT) {
      __sync_fetch_and_sub(&BUCKETS[savedT % (B + 1)], 1);
    } else {
      assert(a <= B + T - 1);
      __sync_fetch_and_sub(&BUCKETS[a % (B + 1)], 1);
    }
    if (T == savedT) {
      break;
    }
    // Revert
    if (a < savedT) {
      __sync_fetch_and_add(&BUCKETS[savedT % (B + 1)], 1);
    } else {
      __sync_fetch_and_add(&BUCKETS[a % (B + 1)], 1);
    }
    // Try again in next iteration
  }
  */
  __sync_fetch_and_sub(&BUCKETS[it->activity % (B + 1)], 1);
  statistics_sanitycheck();
  it->activity = 0;
  smafter = bucketsum();
  assert (smbefore - 1 == smafter);
  pthread_mutex_unlock(&st_lock);
}

void rounder::printStatistics() {
  fprintf(stderr, "hits = %d, misses = %d\n", hits, misses);
  if (requests > 0) {
    assert ( hits + misses == requests );
    fprintf(stderr, "hitratio = %.5f\n", (hits + 0.0) / requests);
    print_buckets();
  }
}

// repetition from basic.hpp :/
void rounder::Miss(const KeyType key) {
  if (p.verbose) {
    printf("Miss(%d)\n", key);
  }
  __sync_fetch_and_add(&misses, 1);
  __sync_fetch_and_add(&requests, 1);
}

/*
// undo delete, required for memcached
void rounder::set_nodel(item *it) {
  act_t savedT, a = it->activity;
  assert(a > 0);
  savedT = T;
  __sync_fetch_and_add(&BUCKETS[(savedT + B - 1) % (B + 1)], 1);
  it->activity = savedT + B - 1;
  while (1) {
    savedT = T;
    if (a < savedT) {
      __sync_fetch_and_sub(&BUCKETS[savedT % (B + 1)], 1);
    } else {
      __sync_fetch_and_sub(&BUCKETS[a % (B + 1)], 1);
    }
    if (T == savedT) {
      break;
    }

    // Revert
    if (a < savedT) {
      __sync_fetch_and_add(&BUCKETS[savedT % (B + 1)], 1);
    } else {
      __sync_fetch_and_add(&BUCKETS[a % (B + 1)], 1);
    }

  }
}
*/

void rounder::statistics_sanitycheck() {
  if (p.verbose) {
    print_buckets();
  }
  assert(1);
}
