#ifndef CONCURRENT_HPP
#define CONCURRENT_HPP

#include <thread>
#include <unistd.h>
#include "common.hpp"
#include "cache_algorithm_base.hpp"


typedef struct _queue_t {
  int **queues;
  int numlines, perqueue;
} queue_t;

cache_algorithm_base *_alg;
programoptions _p;
queue_t _q;

void singlethread_requests(programoptions p, cache_algorithm_base *alg) {
  struct timespec t1, t2;
  unsigned long lines = 0;
  FILE *fp = Fopen(p.tracefilename.c_str(), "r");
  // TODO: mmap
  
  char buf[1024];
  fprintf(stderr, "Starting timer for singlethread experiment\n"); 
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
  item *it;
  for (;fgets(buf, 1024, fp) != NULL;) {
    ++lines;
    char *space = strchrnul(buf, ' ');
    *space = '\0';
    char *key = buf;
    if ((it = alg->get(key)) != NULL) {
      if (p.verbose) {
        printf("HIT(%s)!, it=%p\n", key, it);
      }
    } else {
      if (p.verbose) {
        printf("MISS(%s)\n", key);
      }
      alg->set(key, key);
    }
  }
  fclose(fp);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t2);
  double diff = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1E9;
  printf("Running time: %lfs\n", diff);
  printf("Throughput: %lf OPS/s\n", (lines + 0.0) / diff);
}


#endif // CONCURRENT_HPP
