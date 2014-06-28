
from common import Entry, statbase

class Node(Entry):
    def __init__(self, key, value=None):
        Entry.__init__(self, key, value)
        self.next = None
        self.prev = None

    def __repr__(self):
        return "Node(%s -> %s)" % (self.key, self.value)

class alg:
    def __repr__(self):
        return "CLOCK"

    def __init__(self, c, **kwargs):
        # c is cache size
        self.c = c  # Max
        self.cn = 0 # Current
        self.hand = 0
        self.cache = [] # keys
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
        if e:
            self.stats.Hit(e)
            self.hitcount += 1
            self.stored[key] = e
            return e.value
        self.stats.Miss(key)
        return None

    def put(self, key, value=None):
        e = self.stored.get(key)
        if not e:
            e = Entry(key, value)
            self.stats.Set(e)
            self.stored[key] = e
            if self.cn == self.c:
                i = 0
                while 1:
                    assert i <= self.cn
                    o_key = self.cache[self.hand]
                    o = self.stored[o_key]
                    if o.RUbit == 0:
                        del self.stored[o_key]
                        self.stats.Evict(o)
                        self.cache[self.hand] = key
                        break
                    else:
                        o.RUbit = 0
                    self.hand = (self.hand + 1) % self.cn
                    i += 1
            else:
                # cache not full
                # don't need to evict
                assert self.cn < self.c
                self.cache.append(key)
                self.cn += 1
        else:
            # Overwrite it correctly
            e.value = value
            e.RUbit = 1
        return 1
