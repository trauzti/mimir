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

#define DEBUG_PRINT 0
#define TIME_MAX 31*24*60*60

int R = 100;
float valsum = 0.0;
static size_t hits, misses, requests, PER_BIN;
static double *PLUS, *CDF;
static int *BUCKETS, *TIMECDF;
unsigned int T = 0;
int TOTAL_CACHE_SIZE=0, BITS=0;
pthread_mutex_t age_lock, st_lock;

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
float **ghostplus; // plus[POWER_LARGEST][101];
/*
int *stails; // tail values, need to use % B to get the bucket index
unsigned int **buckets; // buckets[POWER_LARGEST][8];
float **plus; // plus[POWER_LARGEST][101];
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
#if USE_GLOBAL_PLUS
double *global_plus;
#endif
int *accurate_pdf;



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



void statistics_init(int numbuckets, int mR) {
  classstats *cs;

  classes = (classstats *) calloc(POWER_LARGEST, sizeof(classstats));
  ghostplus = (float **) calloc(5, sizeof(float *));
  int clsid;
  for (clsid = 0; clsid < POWER_LARGEST; clsid++) {
    cs = &classes[clsid];
    cs->stail = 0;
    cs->buckets = (unsigned int *)calloc(B, sizeof(unsigned int));
    cs->plus = (double *) calloc(100000, sizeof( double )); // future support
  }
#if USE_GLOBAL_PLUS
  global_plus = calloc(100000, sizeof(double));
  accurate_pdf = calloc(100000, sizeof(int));
#endif
  TOTAL_CACHE_SIZE = 50000;
  printf("statistics_init!\n");
  pthread_mutex_init(&st_lock, NULL);
  pthread_mutex_init(&age_lock, NULL);
  int i;
  misses = 0;
  hits = 0;
  requests = 0;
  PER_BIN = TOTAL_CACHE_SIZE / numbuckets;
  T = 0;
  B = numbuckets;

  printf("Initialized statistics:\n"
      "(TOTAL_CACHE_SIZE, B, PER_BIN )=(%u,%d,%zu)\n",
      TOTAL_CACHE_SIZE, B, PER_BIN);

  CDF = (double *) calloc(TOTAL_CACHE_SIZE + 1, sizeof(double));
  TIMECDF = (int *) calloc(TIME_MAX, sizeof(int)); // one month max
  PLUS = (double *) calloc(TOTAL_CACHE_SIZE + 1, sizeof(double));
  BUCKETS = (int *) calloc(B+1, sizeof(int));
  for (i = 0; i <= B; i++)
  {
    assert (BUCKETS[i] == 0);
  }
  //statistics_sanitycheck();
}

int bucketsum() {
  int i, j,sm = 0;
  for (i = 0; i< B; i++){
    j = BUCKETS[ (T + i) % (B + 1)];
    assert (j >= 0);
    sm += j;
  }
  return sm;
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
  if (BUCKETS[(T + B - 1) % (B+1)] < PER_BIN)
  {
    return false;
  }
  if(0!=pthread_mutex_trylock(&age_lock))
  {
    return false;
  }
  printf("Performing aging\n");
  statistics_sanitycheck();
#if DO_SANITY
  int smbefore, smafter;
  smbefore = bucketsum();
#endif
  BUCKETS[ (T + B) % (B + 1)] = 0; // New top bin to zero
  T++;
  assert (BUCKETS[(T + B - 1) % (B+1)] == 0);
  //BUCKETS[ T % (B + 1)] += BUCKETS[(T-1) % (B+1)]; // Merge bottom bin with old bottom bin
  __sync_fetch_and_add(&BUCKETS[T % (B + 1)], BUCKETS[(T-1) % (B+1)]);
  //TODO: while (!__sync_bool_compare_and_swap());
  printf("Performed aging, range is now [%u,%u]\n", T, T + B - 1);
#if DO_SANITY
  smafter = bucketsum();
  assert (smbefore == smafter);
#endif
  statistics_sanitycheck();
  pthread_mutex_unlock(&age_lock);

  return true;
}

int print_buckets(int clsid) {
  int bucketsum = 0;
  int i;
  classstats *cs = &classes[clsid];
  printf("buckets=[");
  for (i = 0; i < B; i++) {
    int bIndex = (cs->stail + i) % B;
    unsigned int count = GET_count(cs->buckets[bIndex]);
    bucketsum += count;
    printf("%d,", count);
  }
  printf("]\n");

  return bucketsum;
}

void statistics_sanitycheck() {
}


// use: maps the real start and the real end to [0,100]
// post: the plus array has been updated correctly
void update_plus(int clsid, int realstart, int realend) {
  assert(realstart < realend);
  double sz = 0.0 + get_size(clsid);
  int start = (int) (100.0 * (realstart / sz)); // floor
  int end = (int) (0.5 + ( 100.0 *  (realend / sz))); // ceil
#if USE_GLOBAL_PLUS
  int count = realend - realstart;
  double val = 1.0 / (0.0 + count);

  //valsum += 1;
  //global_plus[realstart] += val;
  //global_plus[realend] -= val;

  classes[clsid].plus[realstart] += val;
  classes[clsid].plus[realend] -= val;

  assert (realstart >=0 );
#if DEBUG_PRINT
  int bucketsum = print_buckets(clsid);

  assert (realend <= bucketsum );
#endif
#if DEBUG_PRINT
  printf("realstart, realend, valsum, bucketsum==(%d,%d,%f,%d)\n", realstart, realend, valsum, bucketsum);
#endif
#endif

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
  //update_mapped_plus(clsid, start, end);
}

void update_mapped_plus(int clsid, int start, int end) {
  // Resize the array if end is bigger than the size
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


  // just to check if the threads are the bottleneck
  // XXX YV: removed the 'tid' index to the plus array. threads may become a bottleneck again
  // TS: put the tid val again
  //int tid = get_thread_id();
 // int tid = get_thread_id();
  classes[clsid].plus[start] += val;
  classes[clsid].plus[end] -= val;

  // equivalent to:
  // for (int i = start; i < end; i++) {
  //   pdf[i] += val;
  // }
}

void print_cache_contents(int clsid) {
  item *search = get_head(clsid);
  for(;search != NULL; search=search->next)
  {
    //printf("(%s,%u), ", ITEM_key(search), search->activity);
    printf("%u, ", search->activity);
  }
}

// post: the real stack distance of n is in [d->start,...,d->end-1]
//
void get_stackdistance(item *it, dist *d)
{
  act_t i;
  d->start = 0;
  act_t a=it->activity;

  if(a < T)
    a=T;

  for (i = T + B - 1; i >= T ; --i)
  {
    if (i == a) // go down until we hit the correct region
    {
      break;
    }
    d->start += BUCKETS[i % (B+1)];
  }
  assert (i >= T);

  d->end = d->start + BUCKETS[i % (B+1)];
  d->mid = (d->end - 1 + d->start) / 2.0;
#if DO_SANITY
  printf("start is %d, end is %d, T is %u, B is %u, activity is %u \n",d->start,d->end, T, B, it->activity);
  int t = get_true_stackdistance(it);
  printf("true stackdistance is %d\n,", t);
  assert( d->start <= t);
  assert( t < d->end);
  UDB("Returning stack distance range (start,end)=[%d,%d] (mid=%f) for activity %d\n", d->start, d->end, d->mid, it->activity);
  assert ( 0 <= d->start && d->start <= TOTAL_CACHE_SIZE );
  assert ( 0 <= d->end && d->end <= TOTAL_CACHE_SIZE );
  assert ( d->start < d->end );
#endif
}

int get_true_stackdistance(item *it)
{
  mutex_lock(&cache_lock);
  item *search;
  int j=0;
#if DEBUG_PRINT
  printf("head and down: ");
#endif
  search = get_head(it->slabs_clsid);
  for(;search!=it && search !=0;search=search->next)
  {

#if DEBUG_PRINT
    printf("(%s,%u), ", ITEM_key(search), search->activity);
#endif
    if (search->next != NULL) {
      if (search->activity < search->next->activity) {
#if DEBUG_PRINT
        printf("(%s,%u)\n", ITEM_key(search->next), search->next->activity);
        fflush(stdout);
#endif
        // Perhaps this happens when memcached skips updating a fresh item to the head
        //print_cache_contents(it->slabs_clsid);
        //print_buckets(it->slabs_clsid);
        //assert(0);
      }
      //assert ( search->activity >= search->next->activity);
    }
    j++;
  }
  //fprintf(stderr, "\n");
#if DEBUG_PRINT
  if (search != NULL) {
    printf("(%s,%u)\n", ITEM_key(search), search->activity);
  }
#endif
  if (search ==0 ) {
    assert (j == get_size(it->slabs_clsid));
    assert(0);
  }
  mutex_unlock(&cache_lock);
  return j;
}

void update_pdf(dist *d)
{
/*
  int i;
  double val = 1.0 / (d->end - d->start);
  PLUS[d->start] += val; // Every value at [d.start,...,TOTAL_CACHE_SIZE-1] should be incremented by val
  PLUS[d->end] -= val; // Every value at [d.end,...,TOTAL_CACHE_SIZE-1] should be decremented by val
*/
#if USE_GLOBAL_PLUS
    int count = d->end - d->start;
    double val = 1.0 / (0.0 + count);
    valsum += 1;
    global_plus[d->start] += val;
    global_plus[d->end] -= val;
#endif
}

void statistics_hit(int clsid, item *it) {
  unsigned int timediff = current_time - it->time;
  //  ITEM_UPDATE_INTERVAL is 60 seconds so the diff can be 60 seconds for recently accessed items
  printf("HIT(%s). Least recent access=%u, current_time=%u, diff=%u\n",ITEM_key(it), it->time, current_time, current_time - it->time);
  // TODO: toggle timestamp monitoring on/off with a telnet command
  //       Similar to þ
  // TODO: Export this CDF. This is more useful than our HRC and this is also faster to generate.
  __sync_fetch_and_add(&hits, 1);
  __sync_fetch_and_add(&requests, 1);
  //pthread_mutex_lock(&st_lock);
  assert (it->activity > 0);
  if(it->activity >0) {
    dist d;
    //if (it->activity < T)
    //  it->activity = T;
    statistics_sanitycheck();

    get_stackdistance(it, &d);
    update_pdf(&d);
    if (timediff < TIME_MAX) {
      TIMECDF[timediff]++;
    } else {
      printf("timediff %u > TIME_MAX=%u\n", timediff, TIME_MAX);
    }
    update_plus(clsid, d.start, d.end);
    //BUCKETS[it->activity % (B+1)]--;
    age_if_full(clsid);
    //it->activity = T + B - 1;
    //BUCKETS[it->activity % (B+1)]++;
    statistics_sanitycheck();
  }
  //pthread_mutex_unlock(&st_lock);
}

void statistics_evict(unsigned int clsid, unsigned hv, item *it) {
  act_t savedT, a = it->activity;
  it->activity=0;
  assert (a > 0);
  while (1) {
    savedT = T;
    if (a < savedT)
      __sync_fetch_and_sub(&BUCKETS[savedT % (B+1)], 1);
    else {
      assert(a<=B+T-1);
      __sync_fetch_and_sub(&BUCKETS[a % (B+1)], 1);
    }
    if (T == savedT) break;
    // Revert
    if (a < savedT)
      __sync_fetch_and_add(&BUCKETS[savedT % (B+1)], 1);
    else {
      __sync_fetch_and_add(&BUCKETS[a % (B+1)], 1);
    }
   // Try again in next iteration
  }
  //BUCKETS[it->activity % (B+1)]--;
  it->activity = 0;
}



void statistics_miss(unsigned int clsid, unsigned int hv) {
  __sync_fetch_and_add(&misses, 1);
  __sync_fetch_and_add(&requests, 1);
}

void print_ghosthits() {
  printf("ghosthits=%f\n", ghosthits);
  float f = 0.0;
  int i, t;
  // float cdf[101];
  // memset(cdf, sizeof(float) * 100, 0); // verify!

  for (i = 0 ; i <= 100; i++) {
    for (t = 0; t <= 4; t++) {
      f += ghostplus[t][i];
      //cdf[i] += ghostplus[tid][i]
    }
    //cdf[i+1] += cdf[i];
  }
  printf("f=%f\n", f);
}


void statistics_set(int clsid, item *it) {
  //assert(it->activity==0);
  statistics_sanitycheck();
  // Age other elements (if the first bin is full) before putting this element in the first bin
  age_if_full(clsid);
  //act_t newval = T + B -1;

  it->activity = T + B - 1; // Top val
  __sync_fetch_and_add(&BUCKETS[it->activity % (B+1)],1);
  //__sync_fetch_and_add(&BUCKETS[it->activity % (B+1)], 1); // what if T changes in the meantime ...
  statistics_sanitycheck();
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


