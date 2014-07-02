#ifndef SYSLAB_CACHE_HPP
#define SYSLAB_CACHE_HPP

#include "basic.hpp"
#include "cache_algorithm_base.hpp"
#include "common.hpp"
#include "concurrent.hpp"
#include "lru.hpp"
#include "lru_avl.hpp"
#include "lru_mattson.hpp"
#include "rounder.hpp"
#include "statistics_algorithm_base.hpp"
#include "socket_server.hpp"

cache_algorithm_base *initialize_algorithms(programoptions p) {
  cache_algorithm_base *alg = NULL;
  if (p.cache_algorithm == "LRU_LINKEDLIST") {
    fprintf(stderr, "Using cache algorithm: LRU_LINKEDLIST\n");
    alg = new lru_linkedlist(p);
  } else if (p.cache_algorithm == "LRU_AVL") {
    fprintf(stderr, "Using cache algorithm: LRU_AVL\n");
    alg = new lru_avl(p);
    assert(p.threads == 1);  /* not a thread safe AVL tree */
  } else if (p.cache_algorithm == "LRU_MATTSON") {
    fprintf(stderr, "Using cache algorithm: LRU_MATTSON\n");
    alg = new lru_mattson(p);
    assert(p.threads == 1); /* get_stackdistance is not thread safe */
  } else if (p.cache_algorithm == "CLOCK") {
    // must write this
    assert(0);
    exit(0);
  }

  if (p.statistics_algorithm == "BASIC") {
    fprintf(stderr, "Using statistics algorithm: BASIC\n");
    alg->statistics = new basic(p);
  } else if (p.statistics_algorithm == "ROUNDER") {
    fprintf(stderr, "Using statistics algorithm: ROUNDER\n");
    alg->statistics = new rounder(p);
  } else {
    assert(0);
  }
  return alg;
}


#endif //  SYSLAB_CACHE_HPP
