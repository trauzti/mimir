import random
import time

# Takes k random sample items from the cache and evicts the oldest item
# This is the same algorithm as allkeys-lru in Redis (there k=3 by default)

from common import Entry, statbase

class alg:
    def __repr__(self):
        return "LRU3"

    def __init__(self, c):
        random.seed(1337)
        # c is cache size
        self.c = c
        self.k = 3
        self.cn = 0
        self.cache = [] # (key)
        self.stored = {} # key => entry
        self.hitcount = 0
        self.count = 0
        self.stats = statbase()

    def setup(self, reqlist):
        # I'm an online algorithm :-)
        pass

    def get(self, key):
        self.count += 1
        e = self.stored.get(key)
        if key in self.stored:
            self.stats.Hit(e)
            current_time = time.time()
            e = self.stored[key]
            assert e.timestamp <= current_time
            e.timestamp = current_time
            self.hitcount += 1
            return e.value
        self.stats.Miss(key)
        return None

    def put(self, key, value=None):
        e = self.stored.get(key)
        current_time = time.time()
        if not e:
            # Evict first before inserting (don't want to evict the new item)
            if self.cn == self.c:
                # 0 <= is[0],...,is[self.k-1] <= self.cn-1
                ivals = [ random.randint(0, self.cn-1) for i in xrange(self.k) ]
                oldest_time = float("inf")
                oldest_key = None
                oldest_i = None
                for i in ivals:
                    o_key = self.cache[i]
                    o_e = self.stored[o_key]
                    o_time = o_e.timestamp
                    if o_time < oldest_time:
                        oldest_i = i
                        oldest_time = o_time
                        oldest_key = o_key
                assert oldest_i != None
                assert oldest_key != None
                o = self.stored[oldest_key]
                self.stats.Evict(o)
                del self.stored[oldest_key]
                self.cache[oldest_i] = key
            else:
                self.cn += 1
                self.cache.append(key)
            e = Entry(key, value)
            self.stats.Set(e)
            e.timestamp = current_time
            self.stored[key] = e
            return 1
        else:
            self.stats.Set(e)
            e.timestamp = current_time
            e.value = value
            return 1
