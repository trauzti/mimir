#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

#include "lru_linkedlist.hpp"

using namespace std;

item *lru_linkedlist::get(const char *key) { /* override */
  assert (key >= 0);
  if (p.verbose) {
    printf("get(%d)\n", key);
  }
  item *it = hm.find(key);
  if (it != NULL) {
    // HIT
    statistics->Hit(it, get_stackdistance(it));
    if (strcmp(it->key, key) == 0) {
      unlink(it);
      update_head(it);
    }
  } else {
    // MISS
    statistics->Miss(key);
  }
  return it;
}


void lru_linkedlist::set(const char *key, const char *val) {
  assert (key >= 0);
  if (p.verbose) {
    printf("set(%d)\n", key);
  }
  item *it = hm.find(key);
  assert (it == NULL);
  if (currentsize.load() < p.cachesize) {
    currentsize++;
    it = new item(key, val);
  } else {
    it = evict_tail();
    it->initialize(key, val);
  }
  update_head(it);
  hm.insert(key, it); // this element is outside the linked list
  statistics->Set(it);
}

int lru_linkedlist::get_stackdistance(item *findme) {
  return -1;
}

void lru_linkedlist::unlink(item *it) {
  assert(it != NULL);
  if (it->prev != NULL) it->prev->next = it->next;
  if (it->next != NULL) it->next->prev = it->prev;
  if (it == head) head = it->next;
  if (it == tail) tail = it->prev;
  it->next = NULL;
  it->prev = NULL;
}


void lru_linkedlist::update_head(item *it) {
  assert(it->prev == NULL);
  if (head == NULL) head = it;
  else {
    it->next = head;
    head->prev = it;
    head = it;
  }
  if (tail == NULL) tail = it;
}


item *lru_linkedlist::evict_tail() {
  item *it = NULL;
  // take the tail
  it = tail;
  if (it == NULL) {
    printf("currentsize=%d\n", currentsize.load());
    printCacheContents();
  }
  assert(it != NULL);
  unlink(it);
  hm.erase(it->key);
  statistics->Evict(it);
  return it;
}
