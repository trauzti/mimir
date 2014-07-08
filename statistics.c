#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

#ifdef SYSLAB_CACHE
# include "memcached-orig.h"
#else
# include "memcached.h"
#endif
#include "dablooms.h"
#include "sbuf.h"
#include "statistics.h"
#include "statistics_proto.h"
#include "murmur3_hash.h"


static int R = (1 << 30) - 1; // so we can use & instead of % (because it is faster)

#ifdef SYSLAB_CACHE
const int get_size(int clsid) {
	return 1024;
}

struct {
	int num_threads;
	int maxbytes;
	int chunk_size;
} settings = {
	.num_threads = 1,
	.maxbytes = 0, // must be set somewhere in syslab-cache
	.chunk_size = 1
};

#endif


#define FPP_RATE 0.05 // XXX: YMIR I just changed this in hope of getting more performance (at the cost of accuracy)



// set 0 for the last bucket and make it first
// contaminate last+1 which means mark is 1
// next time we are making last+1 the head, then we set the mark to 0!!!!!!!!
// last, last+1 , ... , head
// tailbucket, tailbucket+1, ..... , firstbucket

classstats *classes;

int failures;
int HeadFilter;
int B; // number of buckets
/*
int *stails; // tail values, need to use % B to get the bucket index
unsigned int **buckets; // buckets[POWER_LARGEST][8];
float **plus; // plus[POWER_LARGEST][101];
float **ghostplus; // plus[POWER_LARGEST][101];
*/
uint32_t **hashes;
#if USE_GLOBAL_FILTER
counting_bloom_t *cfs_global;
unsigned int cfs_global_counter;
#endif
counting_bloom_t **cfs;
float ghosthits;
unsigned int *cfcounters;
int perfilter, ghostlistcapacity;



int get_thread_id() {
#ifdef SYSLAB_CACHE
  return 0; /* one thread */
#else
  int tid = 0;
  pthread_t currid = pthread_self();
  for (; tid < settings.num_threads; tid++) {
    if (pthread_ids[tid] == currid) {
      break;
    }
  }
  /*
  if (tid == settings.num_threads) {
    // called from the main thread
  }
  */
  return tid;
#endif
}



bool atomic_set_and_mark(unsigned int *x, unsigned int exp, unsigned int newv) {
  if(__sync_bool_compare_and_swap(x, exp, newv)) {
    return true;
  }
  return false;
}



void statistics_init(int numbuckets) {
#if USE_ROUNDER
  classstats *cs;
  B = numbuckets;
  printf ("MIMIR loading with %d buckets\n", numbuckets);

  printf("sizeof(item)=%d\n", (int) sizeof(item));
  failures = 0;
  classes = (classstats *) calloc(POWER_LARGEST, sizeof(classstats));
  int clsid;
  for (clsid = 0; clsid < POWER_LARGEST; clsid++) {
    cs = &classes[clsid];
    cs->stail = 0;
    cs->buckets = (unsigned int *)calloc(B, sizeof(unsigned int));
    cs->plus = (float *) calloc(101, sizeof( float )); // so update_plus can decrement plus[100]
    //cs->ghostplus = (float *) calloc(101, sizeof( float )); // NEVER USED
  }
#endif
#if USE_GHOSTLIST
  HeadFilter = 0;
  ghosthits = 0;
  cfs = (counting_bloom_t **) malloc(3 * sizeof(counting_bloom_t *));
  cfcounters = (unsigned int *) calloc(3, sizeof(unsigned int));
  int minsize = (int) ( sizeof(item) + settings.chunk_size);
  printf("min total item size=%d\n",  minsize);
#ifdef SYSLAB_CACHE
  // TODO: change the R from the input settings of syslab-cache
  // R = 10;
  ghostlistcapacity = 200; // XXX HAAAACK
#else
  ghostlistcapacity = settings.maxbytes / minsize;
#endif
  perfilter = ghostlistcapacity / 2;
  printf("ghostlistcapacity=%d\n", ghostlistcapacity);
  printf("perfilter=%d\n", perfilter);
#if USE_GLOBAL_FILTER
  cfs_global = new_counting_bloom(perfilter, FPP_RATE, "/dev/shm/mimir-cfs-global.cf");
  cfs_global_counter = 0;
#endif
  int cfid = 0;
  for(; cfid < 3; cfid++) {
    char fname[128];
    /* Use shared memory instead of disk for speed */
    sprintf(fname, "/dev/shm/mimir-cf%d.cf", cfid);
    cfs[cfid] = new_counting_bloom(perfilter, FPP_RATE, fname);
  }
  int tid = 0;
  hashes = (uint32_t **) calloc(settings.num_threads + 1, sizeof(uint32_t *));
  for (; tid <= settings.num_threads; tid++) {
    hashes[tid] = (uint32_t *) calloc(cfs[0]->nfuncs, sizeof(uint32_t));
  }
#endif
}


void statistics_terminate() {
#if USE_ROUNDER
/* XXX: TO DO
  int clsid;
  free(stails);
  free(buckets);
  return;
  // for some reason this fails
  for (clsid = 0; clsid < POWER_LARGEST; clsid++) {
    free(buckets[clsid]);
  }
*/
#endif
}


// er mark breytt einhvers stadar?
bool age_if_full(int clsid) {
  classstats *cs = &classes[clsid];
  int last = cs->stail % B;
  int head = (cs->stail + B - 1) % B;
  unsigned int head_count = GET_count(cs->buckets[head]);
  unsigned int last_count = GET_count(cs->buckets[last]);
  while (head_count >= (int) (get_size(clsid) / (B +0.0))) {
    int k = (last + 1) % B;
    unsigned int oldcm = cs->buckets[k];
    // when is the mark for the last+1 bucket cleared
    // no thread will execute this loop more than once
    // this is lock free => no thread will be stuck here for an indefinite amount of time
    while (!GET_mark(oldcm)) {
      unsigned int newcm = GET_count_and_mark(GET_count(oldcm) + last_count, 1);
      if (atomic_set_and_mark(&cs->buckets[k], oldcm, newcm)) {
        cs->buckets[last] = 0x0; // set both the value and the mark to 0
        ++cs->stail;
      }
      oldcm = cs->buckets[k]; // other threads will find that mark is 1 and exit
      // only one thread will be able to increment the tail
    }
    return true;
  }
  return false;
}


// use: maps the real start and the real end to [0,100]
// post: the plus array has been updated correctly
void update_plus(int clsid, int realstart, int realend) {
  double sz = 0.0 + get_size(clsid);
  //printf("realstart, realend, sz=(%d,%d,%lf)\n", realstart, realend, sz);
  int start = (int) (100.0 * (realstart / sz)); // floor
  int end = (int) (0.5 + ( 100.0 *  (realend / sz))); // ceil

  if (end == start) {
    ++end;
    //printf("realstart, realend, sz=(%d,%d,%d)\n", realstart, realend,(int) sz);
    //printf("start=%d,end=%d\n", start, end);
    // can happen: realstart, realend, sz=(0,45,10000)
    //assert(0);
  }
  /*
  if (end > 100) {
    //printf("realstart, realend, sz=(%d,%d,%d)\n", realstart, realend, (int) sz);
    //printf("start=%d,end=%d\n", start, end);
    //printf("setting end to 100\n");
    ++failures;
    end = 100;
  }
  if (start < 0) {
    //printf("realstart, realend, sz=(%d,%d,%d)\n", realstart, realend, (int) sz);
    //printf("start=%d,end=%d\n", start, end);
    //printf("setting start to 0\n");
    ++failures;
    start = 0;
  }
  */
  update_mapped_plus(clsid, start, end);
}

void update_mapped_plus(int clsid, int start, int end) {
  if (start < 0 || start > 100) {
    start = 0;
    ++failures;
  }
  if (end < 0 || end > 100) {
    end = 100;
    ++failures;
  }
  if (start >= end) {
    start = 0;
    end = 100;
    ++failures;
  }

  // todo use an array for each thread

  // just checking if this is the bottleneck!!
  // uncomment the plus updates please :) :)
  int count = end - start;
  double val = 1.0 / (0.0 + count);

  //plus[clsid][start] += val;
  //plus[clsid][end] -= val;

  // just to check if the threads are the bottleneck
  // XXX YV: removed the 'tid' index to the plus array. threads may become a bottleneck again
  //int tid = get_thread_id();
  classes[clsid].plus[start] += val;
  classes[clsid].plus[end] -= val;

  // equivalent to:
  // for (int i = start; i < end; i++) {
  //   pdf[i] += val;
  // }
}


void statistics_hit(int clsid, item *e) {
  // TODO: add to concurrent queue and let the background thread do this work
#if USE_ROUNDER
  classstats *cs = &classes[clsid];
  age_if_full(clsid);

  int last = cs->stail;
  if (e->activity < last) {
    e-> activity = last;
  }

  interval_t interval = get_stack_distance(clsid, e->activity);

  update_plus(clsid, interval.start, interval.end);

  remove_from_bucket(clsid, e->activity);
  // we only track the tail and calculate the head if we need it
  int head = add_to_head(clsid);
  e->activity = head;

#endif
}



void statistics_miss(unsigned int clsid, unsigned int hv) {
  return;
  if ( (hv & R) != 0) return; // sampling
#if USE_GHOSTLIST
  //printf("miss(%s)\n", key);
  int tid = get_thread_id();
  // for memcached, reuse the hash value from the hash table! done :-)
  dablooms_hash_func_with_hv(cfs[0], hv, hashes[tid]);

#if USE_GLOBAL_FILTER
  // First see if the element is even in the ghost filter
  if (!counting_bloom_check_with_hash(cfs_global, hashes[tid]))
     return; // Stop early
#endif

  float oldghosthits = ghosthits, newghosthits = ghosthits;
  // Check if the first ghostlist contains this key
  if (counting_bloom_check_with_hash(cfs[HeadFilter % 3], hashes[tid])) {
    // don't change the counters
    newghosthits += 1.0 * (1.0 - FPP_RATE);
    //printf("found in first, ghosthits= %f!\n", ghosthits);
  } else if (counting_bloom_check_with_hash(cfs[(HeadFilter + 1) % 3], hashes[tid])) {
    //cfcounters[(HeadFilter + 1) % 3]--;
    __sync_fetch_and_add(&cfcounters[(HeadFilter + 1) % 3], -1);
    newghosthits += 1.0 * (1.0 - FPP_RATE);
    counting_bloom_remove_with_hash(cfs[(HeadFilter + 1) % 3], hashes[tid]);
    counting_bloom_add_with_hash(cfs[HeadFilter % 3], hashes[tid]);
    //printf("found in second, ghosthits=%f!\n", ghosthits);
  } else if (counting_bloom_check_with_hash(cfs[(HeadFilter + 2) % 3], hashes[tid])) {
    //cfcounters[(HeadFilter + 2) % 3]--;
    __sync_fetch_and_add(&cfcounters[(HeadFilter + 2) % 3], -1);
    counting_bloom_remove_with_hash(cfs[(HeadFilter + 2) % 3], hashes[tid]);
    counting_bloom_add_with_hash(cfs[HeadFilter % 3], hashes[tid]);
    //printf("found in third, ghosthits=%f!\n", ghosthits);
    float prob_in_bounds = 1.0;
    int firsttwo =  cfcounters[HeadFilter % 3] + cfcounters[(HeadFilter + 1) % 3];
    int last = cfcounters[(HeadFilter + 2) % 3];
    if (firsttwo >= ghostlistcapacity) {
      prob_in_bounds =  (0.0 + ghostlistcapacity - firsttwo) / (0.0 + last);
    }
     newghosthits += 1.0 * (1.0 - FPP_RATE) * prob_in_bounds;
  } else {
    //printf("found nowhere :/\n");
    // XXX: (YV) Should we not just return? Clearly this item was not in our ghostlist
    return;
  }
  __sync_bool_compare_and_swap((unsigned int *)&ghosthits, (unsigned int)oldghosthits, (unsigned int)newghosthits);
    // if failed just continue
  //TODO: update the start and end indices
  //ghostplus[tid][0] = ghosthits; // NEVER USED
#endif
}


void statistics_set(int clsid, item *e) {
  // TODO: add to concurrent queue and let the background thread do this work
#if USE_ROUNDER
  age_if_full(clsid);

  int last = classes[clsid].stail;
  if (e->activity < last) {
    e->activity = last;
  }

  int head = add_to_head(clsid); // why set the head ?
  e->activity = head;
#endif
}


void rotateFilters(void) {
#ifndef SYSLAB_CACHE
  printf("rotating filters\n");
#endif
  // TODO: trylock this
  // make sure only one thread does this at once
  int last = (HeadFilter + 2)  % 3;

  memset(cfs[last]->bitmap->array, 0, cfs[last]->bitmap->bytes);
  // make this atomic !!
  cfcounters[last] = 0;
  HeadFilter = (HeadFilter + 2 ) % 3;
}


void statistics_evict(unsigned int clsid, unsigned hv, item *e) {
  return;
  if ((hv & R) != 0) return; // sampling
#if USE_GHOSTLIST
  int tid = get_thread_id();
  //char *key = ITEM_key(e);
  //int nkey = e->nkey;
  //printf("evict(%s)\n", key);
  // get the hv from somewhere, might have to create it :I

  dablooms_hash_func_with_hv(cfs[HeadFilter % 3], hv, hashes[tid]);

#if USE_GLOBAL_FILTER
  // Assume capacity and FPP rate of cfs_global is equal to cfs[i]
  if (!counting_bloom_check_with_hash(cfs_global, hashes[tid])) {
    __sync_fetch_and_add(&cfs_global_counter, 1);
    counting_bloom_add_with_hash(cfs_global, hashes[tid]); // returns 0
  }
  if (cfs_global_counter > perfilter) {
    // Clean the filter
    cfs_global_counter = 0;
    memset (cfs_global->bitmap->array, 0, cfs_global->bitmap->bytes);
  }
#endif

  if (!counting_bloom_check_with_hash(cfs[HeadFilter % 3], hashes[tid])) {
    //cfcounters[HeadFilter % 3]++;
    __sync_fetch_and_add(&cfcounters[HeadFilter % 3], 1);
    counting_bloom_add_with_hash(cfs[HeadFilter % 3], hashes[tid]); // returns 0
  }
  if (cfcounters[HeadFilter % 3] > perfilter) {
    rotateFilters();
  }
#endif


#if USE_ROUNDER
  if (likely (e != NULL))
  {
	  int last = classes[clsid].stail;
	  if (e->activity < last) {
	    e->activity = last;
	  }

	  remove_from_bucket(clsid, e->activity);
  }
#endif
}


// decrements the bucket counter for the given activity
void remove_from_bucket(int clsid, int activity) {
  int i, last; // bucket index
  unsigned int oldcm, newcm;
  while (1) {
    last = classes[clsid].stail;

    if (activity < last) {
      i = last % B;
    } else {
      i = activity % B;
    }

    oldcm = classes[clsid].buckets[i];
    newcm = GET_count_and_mark(GET_count(oldcm) - 1, GET_mark(oldcm));
    if (atomic_set_and_mark(&classes[clsid].buckets[i], oldcm, newcm)) {
      return;
    }
  }
}


// increments the bucket counter for head
int add_to_head(int clsid) {
  classstats *cs = &classes[clsid];
  int head;
  unsigned int oldcm, newcm;
  while (1) {
    head = cs->stail + B - 1;
    oldcm = cs->buckets[head % B];
    newcm = GET_count_and_mark(GET_count(oldcm) + 1, GET_mark(oldcm)); // isn't the mark always 0 for the head bucket?

    // the head value could be incremented in the meantime
    // then we need to try again
    if (atomic_set_and_mark(&cs->buckets[head % B], oldcm, newcm)) {
      return head;
    }
  }
}


interval_t get_stack_distance(int clsid, int activity) {
  classstats *cs = &classes[clsid];
  interval_t ret;
  ret.start = 0;
  ret.end = 0;

  int sz = get_size(clsid);

  unsigned int cm;
  int head  = cs->stail + B - 1;
  // we could get aging in the meantime. therefore we used an intermediate bucket
  // for now we just reset the values
  int last  = cs->stail;
  int i = head;
  int cnt, marked;
  while (1) {
    cm = cs->buckets[i % B];
    cnt = ((int) GET_count(cm) );
    marked = GET_mark(cm);
    assert (cnt >= 0) ;

    if ((i == last || i == activity)) {
	  ret.end = ret.start + cnt;
      break;
    }
    if (marked) {
	  ret.end = sz - 1;
      //printf("hit a marked bucket\n");
      ++failures; // is this a failure?
      break;
    }

    ret.start += cnt;
    --i;
  }
  /*
  if ( ret.start < 0 || ret->end >= 2*sz ) {
    //printf("ret.start=%d, ret.end=%d, get_size(%d)=%d\n", ret.start, ret.end, clsid, sz);
    //printf("i=%d, last=%d, head=%d\n", i, last, head);
    //printf("resetting values\n");
    ret.start = 0;
    ret.end = sz -1;
    ++failures;
    // we hit some concurrency issue. just set the values manually
  }
  */
  return ret;
}



/** Background thread **/

pthread_t mimir_thread_id;
sbuf_t mimir_buffer;

static void *mimir_thread(void *arg)
{
	unsigned int type, keyhash, clsid;

	pthread_detach (mimir_thread_id);

	while (1)
	{
		sbuf_remove (&mimir_buffer, &type, &keyhash, &clsid);
		switch (type)
		{
			case MIMIR_TYPE_EVICT:
				statistics_evict (clsid, keyhash, NULL); // XXX: add support for item parameter
				break;

			case MIMIR_TYPE_MISS:
				statistics_miss (clsid, keyhash);
				break;

			default:
				fprintf (stderr, "ARGH, incorrect type %u received on mimir thread.\n", type);
		}
	}

	return NULL;
}


int start_mimir_thread(void)
{
	int ret;

	sbuf_init (&mimir_buffer, 128*3);

	if ( (ret = pthread_create(&mimir_thread_id, NULL, mimir_thread, NULL)) != 0)
	{
		fprintf (stderr, "Can't start MIMIR thread: %s\n", strerror(ret));
		return -1;
	}

	return 0;
}



int mimir_enqueue(unsigned int type, unsigned int keyhash, unsigned int clsid)
{
	sbuf_insert (&mimir_buffer, type, keyhash, clsid);
	return 0;
}


/* Enqueue with hash lookup attached */
int mimir_enqueue_key(unsigned int type, unsigned int clsid, char *key, size_t keylen)
{
	/* Use the 32-bit murmur hash version used by memcached */
	unsigned int hash = MurmurHash3_x86_32(key, keylen);

	sbuf_insert (&mimir_buffer, type, hash, clsid);
	return 0;
}


