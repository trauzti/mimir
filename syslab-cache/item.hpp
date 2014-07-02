#ifndef ITEM_HPP
#define ITEM_HPP

#include <cstdlib>
#include <atomic>
#include <cstring>
#include <cassert>

using namespace std;

class item {
public:
  char *key = NULL, *value = NULL;
  int timestamp, activity;
  uint8_t nkey;
  item *prev = NULL, *next = NULL;

  item(const char *new_key, const char *new_value) {
    initialize(new_key, new_value);
  }

  void initialize (const char *_key, const char *_value) {
    if (key != NULL) free(key);
    if (value != NULL) free(value);
    nkey = strlen(_key);
    int nval = strlen(_value);
    key = (char *) calloc(1, nkey + 1);
    value = (char *) calloc(1, nval + 1);
    memcpy(key, _key, nkey);
    memcpy(value, _value, nval);
    assert (_key[nkey] == '\0');
    assert (key[nkey] == '\0');
    assert (_value[nval] == '\0');
    assert (value[nval] == '\0');
    assert (strcmp(key, _key) == 0);
    assert (strcmp(value, _value) == 0);
  }

  void initialize (const item &o) {
    initialize(o.key, o.value);
    timestamp = o.timestamp;
  }

  item() : key(NULL), value(NULL) {};

  ~item() {
    free(key);
    free(value);
  }

  // for lru_avl
  friend bool operator< (const item &a, const item &b) {
    if (a.timestamp > b.timestamp) { // inverted because the timestamps are increasing
      return true;
    }
    return false;
  }
  friend bool operator> (const item &a, const item &b) {
    if (a.timestamp < b.timestamp) {
      return true;
    }
    return false;
  }
};

#endif // ITEM_HPP
