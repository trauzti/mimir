#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

#include "lru_avl.hpp"
#include "avl_tree.hpp"
#include "item.hpp"
#include "common.hpp"
#include "statistics_algorithm_base.hpp"
#include "cache_algorithm_base.hpp"

// TODO: support string keys
// Update the PDF and print it in the end

// Reasons for using C++ instead of C:
// 1) no good hashamp in C, google_sparse_hash is in C++, unordered_map is in the standard library for C++11
// 2) cannot compile the libavl AVL tree with g++
// 3) Good AVL tree from SuprDewd (github.com/SuprDewd/CompetitiveProgramming/)
// 4) using C loses the object oriented design niceness

using namespace std;

item *lru_avl::get(const char *key) {
  item *it;
  if ((it = hm.find(key)) != NULL) {
    avlnode *n = tree.find(*it);
    int value = avl_stackdistance(n);
    statistics->Hit(it, value);
    tree.erase(n);
    assert(it != &(n->data));
    update_lru_head(it);
  } else {
    statistics->Miss(key);
  }
  return it;
}


void lru_avl::set(const char *key, const char *value) {
  item *it;
  if ((it = hm.find(key)) != NULL) {
    // overwrite the value, unlink and put it at the head
    it->initialize(key, value);
    avlnode *n = tree.find(*it);
    tree.erase(n);
  } else {
    if (currentsize < p.cachesize) {
      currentsize++;
      it = new item(key, value);
    } else {
      it = evict();
      it->initialize(key, value);
    }
    hm.insert(key, it);
  }
  update_lru_head(it);
}


/* upgrade the timestamp of the item and insert it into the tree */
void lru_avl::update_lru_head(item *it) {
  // increase the timestamp and stamp the item
  timestamp++;
  it->timestamp = timestamp;
  // insert the item into the tree
  tree.insert(*it);
}


/* take the oldest element from the tree and erase it from the hashmap */
item *lru_avl::evict() {
  item *it;
  // find the oldest element in the tree
  avlnode *n = tree.rightmost();
  it = &(n->data);
  const char *key = it->key;

  // find the corresponding item in the hashmap
  item *ithash = hm.find(key);
  if (ithash == NULL) {
    printf("%s,%d not found in the hm\n", it->key, it->timestamp);
    tree.printLRUstack();
    hm.printContents();
    printf("\n\n");
    assert(0);
  }

  statistics->Evict(ithash);
  
  if (strcmp(key, ithash->key) != 0) {
    printf("found %s in hm but expected %s\n", ithash->key, key);
    assert(0);
  }

  // erasing the key from the hashmap
  hm.erase(key);
  // erasing the item from the AVL tree
  assert(it != ithash);
  tree.erase(n);
  // the hashed item is different from the item in the tree
  return ithash;
}


/* the LRU stackdistance of this item */
int lru_avl::get_stackdistance(item *it) {
  avlnode *n = tree.find(*it);
  return avl_stackdistance(n);
}

int lru_avl::avl_stackdistance(avlnode *n) {
  // traverse the tree and count the number of elements younger than this one
  int cl = tree.count_less(n);
#ifdef DEBUG
  //int ld = LD(n) - 1;
  //printf("ld=%d, cl=%d\n", ld, cl);
#endif
  return cl;
  //return LD(n) - 1; // start from 0
}

int lru_avl::avlnodesize(avlnode *n) {
  if (n == NULL) {
    return 0;
  }
  return n->size;
}

int lru_avl::LD(avlnode *n) {
  if (n == NULL) {
    return 0;
  }
  return LD(tree.ANC(n)) + avlnodesize(n->l) + 1;
}
