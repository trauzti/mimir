#include "lru_mattson.hpp"

using namespace std;
int lru_mattson::get_stackdistance(item *findme) {
  // traverse the tree and count the number of elements younger than this one
  item *it = head;
  int dist = 0;
  while (it != NULL && it != findme) {
    it = it->next;
    dist++;
  }
  return dist;
}
