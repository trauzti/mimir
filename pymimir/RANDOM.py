from heapq import heappush, heappop
import random

from common import Entry, statbase

class alg:
    def __repr__(self):
        return "RANDOM"

    def __init__(self, c, **kwargs):
        random.seed(1337)
        # c is cache size
        self.c = c
        self.cn = 0
        self.cache = []
        self.stored = {} # key => item
        self.hitcount = 0
        self.count = 0
        self.stats = statbase()

    def setup(self, reqlist):
        # I'm an online algorithm :-)
        pass

    def get(self, key):
        self.count += 1
        e = self.stored.get(key)
        if e:
            self.stats.Hit(e)
            self.hitcount += 1
            return e.value
        self.stats.Miss(key)
        return None

    def put(self, key, value=None):
        e = self.stored.get(key)
        if not e:
            e = Entry(key, value)
            self.stats.Set(e)
            if self.cn == self.c:
                i = random.randint(0, self.cn-1) # 0 <= i <= (self.cn-1)
                dkey = self.cache[i]
                de = self.stored[dkey]
                self.stats.Evict(de)
                del self.stored[dkey]
                self.cache[i] = key
            else:
                self.cn += 1
                self.cache.append(key)
            self.stored[key] = e
