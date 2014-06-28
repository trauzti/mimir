# Modified from http://code.activestate.com/recipes/576532/
from collections import OrderedDict

from common import Entry, statbase

class Deque:
    'Fast searchable queue'
    def __init__(self):
        self.od = OrderedDict()
    def appendleft(self, k):
        od = self.od
        if k in od:
            del od[k]
        od[k] = None
    def pop(self):
        return self.od.popitem(0)[0]
    def remove(self, k):
        del self.od[k]
    def __len__(self):
        return len(self.od)
    def __contains__(self, k):
        return k in self.od
    def __iter__(self):
        return reversed(self.od)
    def __repr__(self):
        return 'Deque(%r)' % (list(self),)


class alg:
    def __repr__(self):
        return "ARC"

    def __init__(self, c, **kwargs):
        self.c = c        # Cache size
        self.cn = 0       # Items in cache now
        self.cached = {}  # Cached keys
        self.hitcount = 0
        self.count = 0
        self.p = 0
        self.t1 = Deque()
        self.t2 = Deque()
        self.b1 = Deque()
        self.b2 = Deque()
        self.stats = statbase()


    def setup(self, reqlist):
        # I'm an online algorithm :-)
        pass

    def replace(self, args):
        if self.t1 and ((args in self.b2 and len(self.t1) == self.p) or (len(self.t1) > self.p)):
            old = self.t1.pop()
            self.b1.appendleft(old)
        else:
            old = self.t2.pop()
            self.b2.appendleft(old)
        de = self.cached[old]
        self.stats.Evict(de)
        del self.cached[old]

    def get(self, key):
        self.count += 1
        e = None
        if key in self.t1:
            self.t1.remove(key)
            self.t2.appendleft(key)
            self.hitcount += 1
            e = self.cached[key]
            self.stats.Hit(e)
            return e.value
        elif key in self.t2:
            self.t2.remove(key)
            self.t2.appendleft(key)
            self.hitcount += 1
            e = self.cached[key]
            self.stats.Hit(e)
            return e.value
        self.stats.Miss(key)
        return None

    def put(self, key, value=None):
        e = self.cached.get(key)
        if e:
            self.stats.Set(e)
            e.value = value
            return 1
        e = Entry(key, value)
        self.cached[key] = e
        self.stats.Set(e)
        if key in self.b1:
            self.p = min(self.c, self.p + max(len(self.b2) / len(self.b1) , 1))
            self.replace(key)
            self.b1.remove(key)
            self.t2.appendleft(key)
            return 1
        if key in self.b2:
            self.p = max(0, self.p - max(len(self.b1)/len(self.b2) , 1))
            self.replace(key)
            self.b2.remove(key)
            self.t2.appendleft(key)
            return 1
        if len(self.t1) + len(self.b1) == self.c:
            if len(self.t1) < self.c:
                self.b1.pop()
                self.replace(key)
            else:
                popkey = self.t1.pop()
                de = self.cached[popkey]
                self.stats.Evict(de)
                del self.cached[popkey]
        else:
            total = len(self.t1) + len(self.b1) + len(self.t2) + len(self.b2)
            if total >= self.c:
                if total == (2 * self.c):
                    self.b2.pop()
                self.replace(key)
        self.t1.appendleft(key)
        return 1
