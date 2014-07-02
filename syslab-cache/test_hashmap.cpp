#include <thread>
#include <cassert>
#include <ctime>

#include "hashmap.hpp"
#include "common.hpp"

hashmap *hm;
int NUM_THREADS;

#define NUM_REQUESTS 10000000

using namespace std;

void many_requests(int id) {
  struct drand48_data randBuffer;
  long int x;
  uint32_t key;
  bool deletenow;
  char skey[128];
  srand48_r(id + 1, &randBuffer);
  printf("Thread %d starting to perform %d requests!\n", id, NUM_REQUESTS);
  for (int i = 0; i < NUM_REQUESTS; i++) {
    lrand48_r(&randBuffer, &x);
    sprintf(skey, "%d", key);
    key = x % ((1<< 18) -1 );
    ++key; // prevent 0
    item *it; 
    if ((it = hm->find(skey)) == NULL) {
      item *it = new item(skey,skey);
      hm->insert(skey, it);
    } else {
      if (strcmp(skey, it->value) != 0) {
        printf("mismatch %s vs %s\n", skey, it->value);
        assert(0);
      }
    }
  }
  printf("Thread %d done!\n", id);
}


int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s <# threads>\n", argv[0]);
    exit(0);
  }
  NUM_THREADS = atoi(argv[1]);
  srand(1337);

  hm = new hashmap();
  /*
  item *it = new item(1,1);
  hm->insert(1, it);
  item *hashit = hm->find(1);
  assert(hm->find(1) == it);
  printf("Inserting 99 items\n...\r");

  for (int i = 2; i <= 100; i++) {
    it = new item(i, i);
    hm->insert(i, it);
    assert(hm->find(i) == it);
  }
  
  printf("Done!\r\n");
  */
  printf("Spawning %d threads\n", NUM_THREADS);
 
  thread *t = new thread[NUM_THREADS];
  for (int i = 0; i < NUM_THREADS; i++) {
    t[i] = thread(many_requests, i);
  } 
  for (int i = 0; i < NUM_THREADS; i++) {
    t[i].join();
  }
  printf("Hashmap size: %d\n", hm->size());
}
