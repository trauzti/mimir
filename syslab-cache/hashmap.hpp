#ifndef HASHMAP_HPP
#define HASHMAP_HPP

#include <unordered_map>
#include "common.hpp"
#include "item.hpp"
#include <string>

// Just use boost::unordered_map (or tr1 etc) by default. Then profile your code
// and see if that code is the bottleneck. Only then would I suggest to precisely
// analyze your requirements to find a faster substitute.

// Ymir:
// hm => array, linear probing, 
// lock per element, reject or wait

using namespace std;

class hashmap {
public:
  typedef unordered_map<string, item *> internal_hashmap;
  typedef internal_hashmap::iterator hmit;
  internal_hashmap local_hm;

  hashmap() {}

  int size() {
    return local_hm.size();
  }

  item *find(const char *key) {
    string skey(key);
    item *it = NULL;
    hmit got = local_hm.find(skey);
    if (got == local_hm.end()) {
      // not found
      //printf("didn't find %s\n", key);
    } else {
      //printf("found %s\n", key);
      it = got->second;
    }
    return it;
  }

  void erase(const char *key) {
    string skey(key);
    int e = local_hm.erase(skey);
    if (e != 1) {
      printf("Erasing %s failed\n", key);
    } else {
      //printf("Erasing %s suceeded\n", key);
    }
    //assert( e == 1 );
  }

  void insert(const char *key, item *it) {
    string skey(key);
    pair<hmit, bool> x = local_hm.insert(make_pair(skey, it));
    if ( !x.second) {
      printf("Inserting %s failed\n", key);
    } else {
      //printf("Inserting %s suceeded!\n", key);
    }
    //assert(x.second);
  }

  void printContents() {
    int count = 0;
    printf("HashMap:  ");
    for ( auto it = local_hm.begin(); it != local_hm.end(); ++it ) {
      printf("(%s->%p) ", it->first.c_str(), it->second);
      count++;
    }
    printf(" End\n");
    printf("hashmap: %d items\n", count);
  }
};

#endif // HASHMAP_HPP
