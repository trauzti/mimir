#ifndef LRU_AVL_HPP
#define LRU_AVL_HPP

#include "avl_tree.hpp"
#include "lru.hpp"

typedef avl_tree<item>::node avlnode;

class lru_avl : public cache_algorithm_base {
public:
  using cache_algorithm_base::cache_algorithm_base;
  int timestamp;
  avl_tree<item> tree;

  int avlnodesize(avlnode *n);
  int LD(avlnode *n);
  virtual void set(const char *key, const char *val);
  virtual item *get(const char *key);

  item *find(const char *key);
  void update_lru_head(item *it);
  void unlink(item *it);
  item *evict();
  void printCacheContents() {
    printf("head-> ");
    tree.inorder_traversal(tree.root);
    printf(" <- tail\n");
  }
  int avl_stackdistance(avlnode *n);
  virtual int get_stackdistance(item *it);

  virtual ~lru_avl() {
    printf("Destroying lru_avl\n");
  }

};
#endif // LRU_AVL_HPP
