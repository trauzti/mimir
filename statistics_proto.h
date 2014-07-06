/* MIMIR HACK */
#ifndef __STATISTICS_PROTO
#define __STATISTICS_PROTO

#include <pthread.h>
#include <semaphore.h>
#include "sbuf.h"
#include "dablooms.h"

#ifdef __cplusplus
#include "item.hpp"
#endif

/* thread and atomic stuff */
// use: atomically sets the value and mark of a bucket
bool atomic_set_and_mark(unsigned int *x, unsigned int exp, unsigned int newv);
int get_thread_id(void);


typedef struct interval {
  int start;
  int end;
} interval_t;

typedef struct _classstats {
	int stail;
	unsigned int *buckets;
	float *plus;
	//float *ghostplus;
} classstats;


extern int failures;
extern int HeadFilter;
extern int B; // number of buckets
extern int *stails; // tail values, need to use % B to get the bucket index
extern unsigned int **buckets; // buckets[POWER_LARGEST][8]; 
extern float **plus; // plus[POWER_LARGEST][101];
extern float **ghostplus; // plus[POWER_LARGEST][101];
extern uint32_t **hashes;
extern float ghosthits;

/*
 *
// defined in items.c
int get_size(int clsid); 
*/

void statistics_init(int numbuckets);
void statistics_terminate(void);

extern counting_bloom_t **cfs;
extern unsigned int *cfcounters; 
extern int perfilter, ghostlistcapacity;

void rotateFilters(void);
bool age_if_full(int clsid);
void update_plus(int clsid, int realstart, int realend);
void update_mapped_plus(int clsid, int start, int end);
void statistics_hit(int clsid, item *e);
void statistics_set(int clsid, item *e);
void statistics_miss(unsigned int clsid, unsigned int hv);
void statistics_evict(unsigned int clsid, unsigned int hv, item *e);
void remove_from_bucket(int clsid, int activity);
int add_to_head(int clsid);
interval_t get_stack_distance(int clsid, int activity);


/** Background thread **/
extern pthread_t mimir_thread_id;
extern sbuf_t mimir_buffer;
extern int start_mimir_thread(void);
int mimir_enqueue(unsigned int type, unsigned int keyhash, unsigned int clsid);
int mimir_enqueue_key(unsigned int type, unsigned int clsid, char *key, size_t keylen);


#endif
