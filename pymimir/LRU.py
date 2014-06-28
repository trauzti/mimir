import sys

from common import Entry, statbase

class Node(Entry):
    def __init__(self, key, value=None):
        Entry.__init__(self, key, value)
        self.next = None
        self.prev = None

    def __repr__(self):
        return "Node(%s -> %s)" % (self.key, self.value)

class alg:
    def __init__(self, c, **kwargs):
        # c is cache size
        self.c = c
        self.cn = 0
        self.head = None
        self.tail = None
        self.stored = {}
        self.hitcount = 0
        self.count = 0
        self.stats = statbase()

    # Walks the list and asserts that links are correct
    def walk(self):
        n = self.head
        s = "head"
        while n:
            s += "->(%s)" % n.key
            if n.prev:
                assert n.prev.next == n
            if n.next:
                assert n.next.prev == n
            else:
                break
            n = n.next
        s += "<- tail"
        print s
        if n != self.tail:
            from ipdb import set_trace; set_trace()
            print n, self.tail
        assert n == self.tail

    # returns (dist, found)
    # if found == true then the stackdistance of key is in dist and
    # 0 <= dist < c
    # if found == false, the key wasnt found in the cache
    def get_stackdistance(self, key):
        n = self.head
        dist = 0
        found = False
        while n:
            if key == n.key:
                found = True
                break
            n = n.next
            dist += 1
        return (dist, found)

    def __repr__(self):
        return "LRU"

    def setup(self, reqlist):
        # I'm an online algorithm :-)
        pass

    def unlink(self, n):
        if n == self.head:
            self.head = n.next
        if n == self.tail:
            #print "updating tail in unlink"
            self.tail = n.prev
            pass
        if n.prev:
            n.prev.next = n.next
        if n.next:
            n.next.prev = n.prev

    # put n at the LRU head
    def update_head(self, n):
        # Update the LRU head
        n.prev = None
        n.next = self.head
        if self.head:
            self.head.prev = n
        if not self.tail:
            self.tail = n
        self.head = n

    # usage x = get(key)
    # if the key is found in the cache, put it in the LRU head and return the value
    # else return None
    def get(self, key):
        self.count += 1

        if key in self.stored:
            self.hitcount += 1
            n = self.stored[key]
            self.stats.Hit(n)
            if n != self.head:
                self.unlink(n)
                self.update_head(n)
            return n.value
        self.stats.Miss(key)
        return None

    def put(self, key, value=None):
        if key not in self.stored:
            n = Node(key, value)
            self.stats.Set(n)
            self.stored[key] = n
            self.update_head(n)
        else:
            # Overwrite it correctly
            n = self.stored[key]
            self.stats.Set(n)
            n.value = value
            if n != self.head:
                self.unlink(n)
                self.update_head(n)

        if self.cn == self.c:
            # Evict the tail
            dn = self.stored[self.tail.key]
            self.unlink(dn)
            self.stats.Evict(dn)
            del self.stored[dn.key]
        else:
            self.cn += 1
        return 1

    def print_statistics(self):
        print "Hit ratio: %.5f" % (self.hitcount / (0.0 + self.count))
