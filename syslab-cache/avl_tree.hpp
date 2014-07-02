// From: https://raw.github.com/SuprDewd/CompetitiveProgramming/master/code/data-structures/avl_tree.cpp

#ifndef AVL_TREE_HPP
#define AVL_TREE_HPP

#define AVL_MULTISET 0

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <utility>
using namespace std;


template <class T>
class avl_tree {
public:
  struct node {
    T data;
    node *p, *l, *r;
    int size, height;
    node(const T &_data, node *p = NULL) : p(p),
      l(NULL), r(NULL), size(1), height(0) {
        data.initialize(_data);
      }
  };
  avl_tree() : root(NULL) { }
  node *root;

  node *find(const T &data) const {
    node *cur = root;
    while (cur) {
      if (cur->data < data) {
        cur = cur->r;
      } else if (data < cur->data) {
        cur = cur->l;
      } else {
        break;
      }
    }
    return cur;
  }

  node *insert(const T &data) {
    node *prev = NULL, **cur = &root;
    while (*cur) {
      prev = *cur;
      if ((*cur)->data < data) {
        cur = &((*cur)->r);
      }
#if AVL_MULTISET
      else {
        cur = &((*cur)->l);
      }
#else
      else if (data < (*cur)->data) {
        cur = &((*cur)->l);
      } else {
        return *cur;
      }
#endif
    }
    node *n = new node(data, prev);
    *cur = n, fix(n);
    return n;
  }

  void erase(const T &data) {
    erase(find(data));
  }

  void erase(node *n, bool free = true) {
    if (!n) {
      return;
    }
    if (!n->l && n->r) {
      parent_leg(n) = n->r, n->r->p = n->p;
    } else if (n->l && !n->r) {
      parent_leg(n) = n->l, n->l->p = n->p;
    } else if (n->l && n->r) {
      node *s = successor(n);
      erase(s, false);
      s->p = n->p, s->l = n->l, s->r = n->r;
      if (n->l) {
        n->l->p = s;
      }
      if (n->r) {
        n->r->p = s;
      }
      parent_leg(n) = s, fix(s);
      return;
    } else {
      parent_leg(n) = NULL;
    }
    fix(n->p), n->p = n->l = n->r = NULL;
    if (free) {
      delete n;
    }
  }
  node *successor(node *n) const {
    if (!n) {
      return NULL;
    }
    if (n->r) {
      return nth(0, n->r);
    }
    node *p = n->p;
    while (p && p->r == n) {
      n = p, p = p->p;
    }
    return p;
  }

  void printLRUstack() {
    printf("size=%d\n", size());
    inorder_traversal(root);
  }

  void inorder_traversal(node *n) {
    if (n == NULL) {
      return;
    }
    inorder_traversal(n->l);
    printf("(%s,age=%d) ", n->data.key, n->data.timestamp);
    inorder_traversal(n->r);
  }

  node *rightmost() const {
    node *n = root;
    while (n->r != NULL) {
      n = n->r;
    }
    return n;
  }
  node *leftmost() const {
    node *n = root;
    while (n->l != NULL) {
      n = n->l;
    }
    return n;
  }

  node *ANC(node *n) const {
    if (n == NULL || n->p == NULL) {
      return NULL;
    }
    while (n->p->l == n) {
      n = n->p;
      if (n == NULL || n->p == NULL) {
        return NULL;
      }
    }
    return n->p;
  }

  node *predecessor(node *n) const {
    if (!n) {
      return NULL;
    }
    if (n->l) {
      return nth(n->l->size - 1, n->l);
    }
    node *p = n->p;
    while (p && p->l == n) {
      n = p, p = p->p;
    }
    return p;
  }
  inline int size() const {
    return sz(root);
  }
  void clear() {
    delete_tree(root), root = NULL;
  }
  node *nth(int n, node *cur = NULL) const {
    if (!cur) {
      cur = root;
    }
    while (cur) {
      if (n < sz(cur->l)) {
        cur = cur->l;
      } else if (n > sz(cur->l)) {
        n -= sz(cur->l) + 1, cur = cur->r;
      } else {
        break;
      }
    }
    return cur;
  }
  int count_less(node *cur) {
    int sum = sz(cur->l);
    while (cur) {
      if (cur->p && cur->p->r == cur) sum += 1 + sz(cur->p->l);
      cur = cur->p;
    } return sum; 
  }
private:
  inline int sz(node *n) const {
    return n ? n->size : 0;
  }
  inline int height(node *n) const {
    return n ? n->height : -1;
  }
  inline bool left_heavy(node *n) const {
    return n && height(n->l) > height(n->r);
  }
  inline bool right_heavy(node *n) const {
    return n && height(n->r) > height(n->l);
  }
  inline bool too_heavy(node *n) const {
    return n && abs(height(n->l) - height(n->r)) > 1;
  }
  void delete_tree(node *n) {
    if (n) {
      delete_tree(n->l), delete_tree(n->r);
      delete n;
    }
  }
  node *&parent_leg(node *n) {
    if (!n->p) {
      return root;
    }
    if (n->p->l == n) {
      return n->p->l;
    }
    if (n->p->r == n) {
      return n->p->r;
    }
    assert(false);
  }
  void augment(node *n) {
    if (!n) {
      return;
    }
    n->size = 1 + sz(n->l) + sz(n->r);
    n->height = 1 + max(height(n->l), height(n->r));
  }
#define rotate(l, r) \
        node *l = n->l; \
        l->p = n->p; \
        parent_leg(n) = l; \
        n->l = l->r; \
        if (l->r) l->r->p = n; \
        l->r = n, n->p = l; \
        augment(n), augment(l)
  void left_rotate(node *n) {
    rotate(r, l);
  }
  void right_rotate(node *n) {
    rotate(l, r);
  }
  void fix(node *n) {
    while (n) {
      augment(n);
      if (too_heavy(n)) {
        if (left_heavy(n) && right_heavy(n->l)) {
          left_rotate(n->l);
        } else if (right_heavy(n) && left_heavy(n->r)) {
          right_rotate(n->r);
        }
        if (left_heavy(n)) {
          right_rotate(n);
        } else {
          left_rotate(n);
        }
        n = n->p;
      }
      n = n->p;
    }
  }
};

#endif
