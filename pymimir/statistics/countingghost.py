import pydablooms
import smhasher
import random
from math import ceil

FPP_RATE = 0.01

class ghostclass:
    cfs = []
    counters = []

    # eviction in the main cache
    # use: g = ghostclass(capacity=capacity)
    def __init__(self, R=1, capacity=0, filepath="ghostlist.bf"):
        self.filepath = filepath
        self.capacity_per_filter = int(ceil(0.5 * capacity))
        for i in xrange(3):
            cf = pydablooms.Dablooms(capacity=self.capacity_per_filter, error_rate=FPP_RATE, filepath="%d_%s" %(i, self.filepath))
            self.cfs.append(cf)
            self.counters.append(0)
        #print self.cfs
        self.total_count = 0
        self.first = 0
        self.capacity = capacity
        #print "capacity_per_filter=%d, but capacity=%d" % (self.capacity_per_filter, self.capacity)
        self.ghosthits = 0
        self.ghostmisses = 0
        self.rotations = 0
        self.R = R
        print "R=%d" % R
        self.Rignored = 0
        self.Rpassed = 0
        self.ghostplus = [0.0 for c in xrange(capacity)]
        self.pdf = [0.0 for j in xrange(int(ceil(1.5*capacity)))]

    # miss in the main cache
    # use: x = Miss(key)
    # post: x == True <==> key was found in the ghostlist
    def Miss(self, key):
        hv = smhasher.murmur3_x64_64(key)
        if (hv % self.R) != 0:
            self.Rignored += 1
            return
        else:
            self.Rpassed += 1
        found = False
        start = 0
        end = 0
        val = 0
        first = self.first % 3
        second = (self.first + 1) % 3
        last = (self.first + 2) % 3

        for index in [first, second, last]:
            end += self.R * self.counters[index]
            if self.cfs[index].check(key):
                found = True
                break
            start += self.R * self.counters[index]

        if found:
            if end > self.capacity:
                if start < self.capacity:
                    # This is overestimating values from [capacity,...,1.5*capacity]
                    # hence scale down
                    x = self.R * (1.0 - FPP_RATE) * (end - self.capacity + 0.0) / (end - start + 0.0)
                    assert x >= 0
                    self.ghosthits += x
                else:
                    self.ghosthits += 0  # hit at stack distance greater than the capacity
            else:
                self.ghosthits += self.R * (1.0 - FPP_RATE)
            val = self.R * ( 1.0 / (end - start) ) * (1.0 - FPP_RATE)
            for i in xrange(start, min(int(1.5*self.capacity), end)):
                self.pdf[i] += val
        else:
            self.ghostmisses += 1

        return found

    # eviction in the main cache
    # use: Evict(key)
    def Evict(self, key):
        hv = smhasher.murmur3_x64_64(key)
        #print "Evict", hv
        if (hv % self.R) != 0:
            return
        if self.R * self.counters[self.first] >= self.capacity_per_filter:
            self.rotateFilters()
        if not self.cfs[self.first].check(key):
            #print "inserting %s into the first filter" % key
            self.cfs[self.first].add(key, 0)
            self.counters[self.first] += 1

    def rotateFilters(self):
        assert self.R * self.counters[self.first] >= self.capacity_per_filter
        last = (self.first + 2) % 3
        # clear the last filter and make it an empty first filter
        self.cfs[last] = pydablooms.Dablooms(capacity=self.capacity_per_filter, error_rate=FPP_RATE, filepath="%d_%s" %(last, self.filepath))
        self.counters[last] = 0
        self.first = last
        self.rotations += 1
        #print "Done rotating filters!"

    def printCounters(self):
        print self.counters
        s = ""
        print "first=%d" % self.first

        s = "first_to_last = ["
        for i in xrange(3):
            index = (self.first + i) % 3
            if i > 0:
                s += ","
            s+= "%d" % self.counters[i]
        s += "]"
        print s

    def make_pdf(self):
        pass

    def printStatistics(self, filename=None):
        self.printCounters()

        realghosthits = sum([self.pdf[i] for i in xrange(self.capacity)])
        s = "Rignored=%d vs Rpassed=%d\n" % (self.Rignored, self.Rpassed)
        s += "ghosthits=%.3f , realghosthits=%.3f\n" % (self.ghosthits, realghosthits)
        s += "ghostmisses=%d\n" % self.ghostmisses
        s += "rotations=%d\n" % self.rotations

        if filename:
            f = open(filename, "w")
            f.write(s)
            f.close()
        else:
            print s
