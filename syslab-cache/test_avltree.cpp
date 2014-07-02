#include <thread>
#include <cassert>
#include <ctime>

#include "lru_avl.hpp"
#include "syslab_cache.hpp"

hashmap *hm;
int NUM_THREADS;

#define NUM_REQUESTS 10000000

using namespace std;

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s <# items>\n", argv[0]);
    exit(0);
  }
  programoptions p;
  p.cachesize = 1000;
  p.cache_algorithm = "LRU_AVL";
  p.statistics_algorithm = "BASIC";

  cache_algorithm_base *alg = initialize_algorithms(p);
  
  printf("setting\n");
  alg->set("100", "1000");
  printf("getting\n");
  item *y = alg->get("100");
}
