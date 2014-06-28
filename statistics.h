#ifndef STATISTICS_H
#define STATISTICS_H


#include <stdlib.h>
#include <stdio.h>

//#include "item.hpp"
#include "dablooms.h"
#include "memcached.h"

#define USE_GHOSTLIST 0
#define USE_ROUNDER 1

// two things: if we had 2 value cas we wouldn't need it (DCAS)
// wrapping the value and the mark into one struct to be able to use a single CAS instruction


// each bucket contains a mark and a counter
// the mark denotes whether some thread is doing aging
// the counter is the number of entries in the bucket

#define GET_mark(cm) ((cm) & 0x1)
#define GET_count(cm) ((cm) >> 1)
#define GET_count_and_mark(count, mark) (((count) << 1 ) | ((mark) & 0x1)) // just to make sure that mark is 0 or 1
#define SET_count_and_mark(x, count, mark) ( x = GET_count_and_mark(count, mark))


/* thread and atomic stuff */
// use: atomically sets the value and mark of a bucket
bool atomic_set_and_mark(unsigned int *x, unsigned int exp, unsigned int newv);
int get_thread_id(void);


typedef struct interval {
  int start;
  int end;
} interval_t;


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
void statistics_miss(int clsid, const char *key);
void statistics_miss(int clsid, const char *key, uint32_t hv, int nkey);
void statistics_set(int clsid, item *e);
void statistics_evict(int clsid, item *e);
void remove_from_bucket(int clsid, int activity);
int add_to_head(int clsid);
interval_t get_stack_distance(int clsid, int activity);

#endif
