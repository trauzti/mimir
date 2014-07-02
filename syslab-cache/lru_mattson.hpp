#ifndef LRU_MATTSON_HPP
#define LRU_MATTSON_HPP

#include "lru_linkedlist.hpp"

class lru_mattson : public lru_linkedlist {
  using lru_linkedlist::lru_linkedlist;
  using lru_linkedlist::get;
  using lru_linkedlist::set;
  using lru_linkedlist::update_head;
  using lru_linkedlist::unlink;
  using lru_linkedlist::evict_tail;
  using lru_linkedlist::printCacheContents;
  virtual int get_stackdistance(item *findme);
  virtual ~lru_mattson() {
    printf("Destroying lru_mattson\n");
  }
};

#endif // LRU_MATTSON_HPP
