from heapq import heappush, heappop

from common import Entry

class alg:
    def __repr__(self):
        return "LFU"

    def __init__(self, c, **kwargs):
        # c is cache size
        self.c = c
        self.cn = 0
        self.stored = {}
        self.entries = {} # key => entry
        self.heap = []  # (dist, key, valid)
        self.hitcount = 0
        self.count = 0

    def setup(self, reqlist):
        # I'm an online algorithm :-)
        pass

    def get(self, key):
        self.count += 1
        if key in self.stored:
            self.hitcount += 1
            old = self.stored[key]
            old[2] = False
            item = [old[0]+1, key, True]
            heappush(self.heap, item)
            e = self.entries[key]
            self.stored[key] = item
            self.stats.Hit(e)
            return e.value
        self.stats.Miss(key)
        return None

    def put(self, key, value=None):
        if key not in self.stored:
            if self.cn == self.c:
                valid = False
                delkey = None
                while not valid:
                    dist, delkey, valid = heappop(self.heap)
                del self.stored[delkey]
                dn = self.entries[delkey]
                self.stats.Evict(dn)
                del self.entries[delkey]
            else:
                self.cn += 1
            item = [1, key, True]
            heappush(self.heap, item)
            self.stored[key] = item
            e = Entry(key, value)
            self.stats.Set(e)
            self.entries[key] = e
            return 1
        else:
            # TODO: Overwrite it correctly
            return 0
