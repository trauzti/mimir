#ifndef LRU_LINKEDLIST_HPP
#define LRU_LINKEDLIST_HPP

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

#include "item.hpp"
#include "common.hpp"
#include "cache_algorithm_base.hpp"


using namespace std;

class lru_linkedlist : public cache_algorithm_base {
public:
  item *head, *tail;
  pthread_mutex_t list_lock;

  // the linked list is allocated as the cache grows
  // but when the cache is full elements are reused
  //  ===========

  using cache_algorithm_base::cache_algorithm_base;

  lru_linkedlist(programoptions p) : cache_algorithm_base(p) {
    head = NULL;
    tail = NULL;
  }

  virtual void set(const char *key, const char *val);
  virtual item* get(const char *key);
  void update_head(item *it);
  void unlink(item *it);
  item *evict_tail();

  void printCacheContents() {
    printf("LinkedList head-> ");
    item *it = head;
    while (it != NULL) {
      printf("(%p->%d) ", it, it->key);
      it = it->next;
    }
    printf(" <- tail\n");
  }

  virtual int get_stackdistance(item *findme);
  virtual ~lru_linkedlist() {
    printf("Destroying lru_linkedlist\n");
    item *it = head, *last = NULL;
    while (it != NULL) {
      last = it;
      delete last;
      it = it->next;
    }
  }
};

#endif // LRU_LINKEDLIST_HPP
