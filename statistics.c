#include <assert.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

#include "memcached.h"
#include "dablooms.h"
#include "sbuf.h"
#include "statistics_proto.h"
#include "murmur3_hash.h"




// set 0 for the last bucket and make it first
// contaminate last+1 which means mark is 1
// next time we are making last+1 the head, then we set the mark to 0!!!!!!!!
// last, last+1 , ... , head
// tailbucket, tailbucket+1, ..... , firstbucket

#define FPP_RATE 0.01
int failures;
int HeadFilter;
int B; // number of buckets
int *stails; // tail values, need to use % B to get the bucket index
unsigned int **buckets; // buckets[POWER_LARGEST][8];
float **plus; // plus[POWER_LARGEST][101];
float **ghostplus; // plus[POWER_LARGEST][101];
uint32_t **hashes;
float ghosthits;

counting_bloom_t **cfs;
unsigned int *cfcounters;
int perfilter, ghostlistcapacity;

int get_thread_id() {
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
}

bool atomic_set_and_mark(unsigned int *x, unsigned int exp, unsigned int newv) {
  if(__sync_bool_compare_and_swap(x, exp, newv)) {
    return true;
  }
  return false;
}

void statistics_init(int numbuckets) {
#if USE_ROUNDER
  B = numbuckets;
  printf("sizeof(item)=%d\n", (int) sizeof(item));
  failures = 0;
  buckets = (unsigned int **) calloc(POWER_LARGEST, sizeof(unsigned int *));
  stails = (int *) calloc(POWER_LARGEST, sizeof(int));
  plus = (float **) calloc(POWER_LARGEST, sizeof( float *));
  ghostplus = (float **) calloc(POWER_LARGEST, sizeof( float *));
  int clsid;
  for (clsid = 0; clsid < POWER_LARGEST; clsid++) {
    buckets[clsid] = (unsigned int *)calloc(B, sizeof(unsigned int));
    stails[clsid] = 0;
    plus[clsid] = (float *) calloc(101, sizeof( float )); // so update_plus can decrement plus[100]
    ghostplus[clsid] = (float *) calloc(101, sizeof( float ));
  }
#endif
#if USE_GHOSTLIST
  HeadFilter = 0;
  ghosthits = 0;
  cfs = (counting_bloom_t **) malloc(3 * sizeof(counting_bloom_t *));
  cfcounters = (unsigned int *) calloc(3, sizeof(unsigned int));
  int minsize = (int) ( sizeof(item) + settings.chunk_size);
  printf("min total item size=%d\n",  minsize);
  ghostlistcapacity = settings.maxbytes / minsize;
  perfilter = ghostlistcapacity / 2;
  printf("ghostlistcapacity=%d\n", ghostlistcapacity);
  printf("perfilter=%d\n", perfilter);
  int cfid = 0;
  for(; cfid < 3; cfid++) {
    char fname[128];
    sprintf(fname, "cf%d.cf", cfid);
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
  int clsid;
  free(stails);
  free(buckets);
  return;
  // for some reason this fails
  for (clsid = 0; clsid < POWER_LARGEST; clsid++) {
    free(buckets[clsid]);
  }
#endif
}


// er mark breytt einhvers stadar?
bool age_if_full(int clsid) {
  int last = stails[clsid] % B;
  int head = (stails[clsid] + B - 1) % B;
  unsigned int head_count = GET_count(buckets[clsid][head]);
  unsigned int last_count = GET_count(buckets[clsid][last]);
  while (head_count >= (int) (get_size(clsid) / (B +0.0))) {
    int k = (last + 1) % B;
    unsigned int oldcm = buckets[clsid][k];
    // when is the mark for the last+1 bucket cleared
    // no thread will execute this loop more than once
    // this is lock free => no thread will be stuck here for an indefinite amount of time
    while (!GET_mark(oldcm)) {
      unsigned int newcm = GET_count_and_mark(GET_count(oldcm) + last_count, 1);
      if (atomic_set_and_mark(&buckets[clsid][k], oldcm, newcm)) {
        buckets[clsid][last] = 0x0; // set both the value and the mark to 0
        ++stails[clsid];
      }
      oldcm = buckets[clsid][k]; // other threads will find that mark is 1 and exit
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
  int tid = get_thread_id();

  // todo use an array for each thread

  // just checking if this is the bottleneck!!
  // uncomment the plus updates please :) :)
  int count = end - start;
  double val = 1.0 / (0.0 + count);

  //plus[clsid][start] += val;
  //plus[clsid][end] -= val;

  // just to check if the threads are the bottleneck
  plus[tid][start] += val;
  plus[tid][end] -= val;

  // equivalent to:
  // for (int i = start; i < end; i++) {
  //   pdf[i] += val;
  // }
}


void statistics_hit(int clsid, item *e) {
  // TODO: add to concurrent queue and let the background thread do this work
#if USE_ROUNDER
  age_if_full(clsid);

  int last = stails[clsid];
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
  
#if USE_GHOSTLIST
  //printf("miss(%s)\n", key);
  int tid = get_thread_id();
  // for memcached, reuse the hash value from the hash table! done :-)
  dablooms_hash_func_with_hv(cfs[0], hv, hashes[tid]);

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
  }
  __sync_bool_compare_and_swap(&ghosthits, oldghosthits, newghosthits);
    // if failed just continue
  //TODO: update the start and end indices
  ghostplus[tid][0] = ghosthits;
#endif
}


void statistics_set(int clsid, item *e) {
  // TODO: add to concurrent queue and let the background thread do this work
#if USE_ROUNDER
  age_if_full(clsid);

  int last = stails[clsid];
  if (e->activity < last) {
    e->activity = last;
  }

  int head = add_to_head(clsid); // why set the head ?
  e->activity = head;
#endif
}


void rotateFilters(void) {
  printf("rotating filters\n");
  // TODO: trylock this
  // make sure only one thread does this at once
  int last = (HeadFilter + 2)  % 3;

  memset(cfs[last]->bitmap->array, 0, cfs[last]->bitmap->bytes);
  // make this atomic !!
  cfcounters[last] = 0;
  HeadFilter = (HeadFilter + 2 ) % 3;
}


void statistics_evict(unsigned int clsid, unsigned hv) {
#if USE_GHOSTLIST
  //int tid = get_thread_id();
  //char *key = ITEM_key(e);
  //int nkey = e->nkey;
  //printf("evict(%s)\n", key);
  // get the hv from somewhere, might have to create it :I

  dablooms_hash_func_with_hv(cfs[HeadFilter % 3], hv, hashes[tid]);
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
// XXX MIMIR (YV): I removed the following code. I suspect it's important. Can you double check?
  /*
  int last = stails[clsid];
  if (e->activity < last) {
    e->activity = last;
  }

  remove_from_bucket(clsid, e->activity);
  */
#endif
}


// decrements the bucket counter for the given activity
void remove_from_bucket(int clsid, int activity) {
  int i, last; // bucket index
  unsigned int oldcm, newcm;
  while (1) {
    last = stails[clsid];

    if (activity < last) {
      i = last % B;
    } else {
      i = activity % B;
    }

    oldcm = buckets[clsid][i];
    newcm = GET_count_and_mark(GET_count(oldcm) - 1, GET_mark(oldcm));
    if (atomic_set_and_mark(&buckets[clsid][i], oldcm, newcm)) {
      return;
    }
  }
}


// incremenets the bucket counter for head
int add_to_head(int clsid) {
  int head;
  unsigned int oldcm, newcm;
  while (1) {
    head = stails[clsid] + B - 1;
    oldcm = buckets[clsid][head % B];
    newcm = GET_count_and_mark(GET_count(oldcm) + 1, GET_mark(oldcm)); // isn't the mark always 0 for the head bucket?

    // the head value could be incremented in the meantime
    // then we need to try again
    if (atomic_set_and_mark(&buckets[clsid][head % B], oldcm, newcm)) {
      return head;
    }
  }
}


interval_t get_stack_distance(int clsid, int activity) {
  interval_t ret;
  ret.start = 0;
  ret.end = 0;

  int sz = get_size(clsid);

  unsigned int cm;
  int head  = stails[clsid] + B - 1;
  // we could get aging in the meantime. therefore we used an intermediate bucket
  // for now we just reset the values
  int last  = stails[clsid];
  int i = head;
  int cnt, marked;
  while (1) {
    cm = buckets[clsid][i % B];
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

	/* XXX Detach thread */
	while (1)
	{
		sbuf_remove (&mimir_buffer, &type, &keyhash, &clsid);
		switch (type)
		{
			case MIMIR_TYPE_EVICT:
				statistics_evict (clsid, keyhash);
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

	sbuf_init (&mimir_buffer, 16384);

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



int mimir_enqueue_key(unsigned int type, unsigned int clsid, char *key, size_t keylen) 
{
	/* Use the 32-bit murmur hash version used by memcached */
	unsigned int hash = MurmurHash3_x86_32(key, keylen);

	sbuf_insert (&mimir_buffer, type, hash, clsid);
	return 0; 
}


