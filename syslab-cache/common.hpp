#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <atomic>
#include <string>
#include <vector>
#include <omp.h>

#define MIN_BUCKETS 2
#define MAX_BUCKETS 128

#define DefaultCDFsize 100 

#ifdef _OPENMP
  #define MAX_THREADS (omp_get_max_threads())
#else
  #define MAX_THREADS 1
#endif

#define NUM_REFLOCKS 8191


using namespace std;
void print_array(string msg, vector<string> v);

/*
void *xcalloc(unsigned long size, unsigned long nmemb) {
  void *ret;
  if ((ret = calloc(nmemb, size)) == NULL) {
    exit(EXIT_FAILURE);
  } else {
    return ret;
  }
}
*/



FILE *Fopen(const char *filename, const char *mode);
int CheckFile(const char *filename, const char *mode);


class dist {
public:
  int start, mid, end;
};

class programoptions {
public:
  int simulation_mode, cachesize, cdfsize, verbose, threads;
  int statistics_buckets;
  string cache_algorithm, statistics_algorithm;
  string tracefilename, cdffilename;
  bool writecdf = false;

  programoptions() :  simulation_mode(1), cachesize(0), cdfsize(DefaultCDFsize),
    verbose(0), threads(1), statistics_buckets(8),
    cache_algorithm("LRU_LINKEDLIST"), statistics_algorithm("BASIC"), 
    cdffilename("cdf.txt") {}

  int validate() {
    int ret = 1;
    ret &= CheckFile(tracefilename.c_str(), "r");
    ret &= CheckFile(cdffilename.c_str(), "w");
    if (cachesize < 1) {
      fprintf(stderr,
              "      cachesize must be >= 1\n");
      ret = 0;
    }
    if (cdfsize < 1) {
      fprintf(stderr,
              "      cdfsize must be >= 1\n");
      ret = 0;
    }
    if (threads < 1 || threads > MAX_THREADS) {
      fprintf(stderr,
              "      Number of threads should be in the range [1,%d]\n", MAX_THREADS);
      ret = 0;
    }
    if (statistics_buckets < MIN_BUCKETS || statistics_buckets > MAX_BUCKETS) {
      fprintf(stderr,
              "      Number of buckets should be in the range [%d,%d]\n", MIN_BUCKETS, MAX_BUCKETS);
      ret = 0;
    }
    vector<string> valid_cachealgorithms {"LRU_LINKEDLIST", "LRU_MATTSON", "LRU_AVL", "CLOCK"};
    vector<string>::iterator it = find(valid_cachealgorithms.begin(), valid_cachealgorithms.end(), cache_algorithm);
    if (it == valid_cachealgorithms.end()) {
      print_array("the cache algorithm is not in", valid_cachealgorithms);
      ret = 0;
    }
    vector<string> valid_statisticsalgorithms {"BASIC", "ROUNDER"};
    it = find(valid_cachealgorithms.begin(), valid_cachealgorithms.end(), statistics_algorithm);
    if (it == valid_statisticsalgorithms.end()) {
      print_array("the statistics algorithm is not in", valid_statisticsalgorithms);
      ret = 0;
    }
    return ret;
  }

  void usage(char *filename) {
    printf("Usage: %s\n"
           "     -h                         display this message\n"
           "     -v                         verbose\n"
           "     -m                         simulation mode, no data is stored\n"
           "     -w                         write the cdf to a file\n"
           "     -f <filename>              the input tracefile\n"
           "     -o <filename>              the output cdffile\n"
           "     -t <threads>               the number of threads\n"
           "     -n <cache_size>            cache size (number of elements)\n"
           "     -l <cdf_size>              cdf size (number of elements)\n"
           "     -c <cache_algorithm>       cache algorithm\n"
           "     -t <threads>               number of threads\n"
           "     -b <buckets>               number of buckets for statistics (default 8)\n"
           "     -s <statistics_algorithm>  statistics algorithm\n",
           filename);
  }

};

#endif //  COMMON_HPP
