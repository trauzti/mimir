#include <thread>
#include <cassert>
#include <ctime>

#include "hashmap.hpp"
#include "lru_linkedlist.hpp"
#include "common.hpp"
#include "rounder.hpp"

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

  cache_algorithm_base *x = new lru_linkedlist(p);
  statistics_algorithm_base *r = new rounder(p);
  x->set("100", "1000");
  item *y = x->get("100");
}
