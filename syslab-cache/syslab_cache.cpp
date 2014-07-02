#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <getopt.h>
#include <string>

#include "syslab_cache.hpp"

programoptions parse_programoptions(int argc, char **argv) {
  programoptions p;
  int c;
  while (-1 != (c = getopt(argc, argv,
                           "h"  /* display the help message */
                           "v"  /* verbose */
                           "m"  /* simulation mode */
                           "w"  /* write the cdf to a file */
                           "f:"  /* filename */
                           "o:"  /* cdf filename */
                           "n:" /* cache size */
                           "l:" /* cdf size */
                           "c:" /* cache algorithm */
                           "t:" /* number of threads */
                           "b:" /* number of buckets for statistics */
                           "s:" /* statistics algorithm */
                          ))) {
    switch (c) {
    case 'h':
      p.usage(argv[0]);
      exit(0);
      break;
    case 'v':
      p.verbose = 1;
      break;
    case 'm':
      p.simulation_mode = 1;
      break;
    case 'w':
      p.writecdf = true;
      break;
    case 'f':
      p.tracefilename = string(optarg);
      break;
    case 'o':
      p.cdffilename = string(optarg);
      break;
    case 'n':
      p.cachesize = atoi(optarg);
      break;
    case 'l':
      p.cdfsize = atoi(optarg);
      break;
    case 'c':
      p.cache_algorithm = string(optarg);
      break;
    case 't':
      p.threads = atoi(optarg);
      break;
    case 'b':
      p.statistics_buckets = atoi(optarg);
      break;
    case 's':
      p.statistics_algorithm = string(optarg);
      break;
    default:
      break;
    }
  }
  return p;
}


int main(int argc, char **argv) {
  programoptions p = parse_programoptions(argc, argv);
  if (!p.validate()) {
    fprintf(stderr, "%s -h for usage\n", argv[0]);
    return EXIT_FAILURE;
  }

  cache_algorithm_base *alg = initialize_algorithms(p);
  singlethread_requests(p, alg);

  alg->statistics->printStatistics();
  
  if (p.writecdf) {
    if (alg->statistics->make_cdf()) {
      alg->statistics->write_cdf(p.cdffilename.c_str());
    }
  }
  return 0;
}
