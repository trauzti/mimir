import pydablooms
import random
from math import ceil

FPP_RATE = 0.01

class ghostclass:
    cfs = []
    counters = []

    # eviction in the main cache
    # use: g = ghostclass(capacity=capacity)
    def __init__(self, capacity=0, filepath="ghostlist.bf"):
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
        self.ghostplus = [0.0 for c in xrange(capacity)]
        self.pdf = [0.0 for j in xrange(int(ceil(1.5*capacity)))]

    # miss in the main cache
    # use: x = Miss(key)
    # post: x == True <==> key was found in the ghostlist
    def Miss(self, key):
        found = False
        start = 0
        end = 0
        val = 0
        for i in xrange(3):
            index = (self.first + i) % 3
            end += self.counters[index]
            if self.cfs[index].check(key):
                if i == 2:
                    second = (self.first + 1) % 3
                    last = (self.first + 2) % 3
                    firsttwo_contain = self.counters[self.first] + self.counters[second]
                    if sum(self.counters) <= self.capacity:
                        self.ghosthits += 1.0 * (1.0 - FPP_RATE)
                    elif (firsttwo_contain < self.capacity) and self.counters[last] > 0:
                        # the first two filters are too small to cover up to 2x
                        # the element is in the last filter and could have been a hit in a cache of size 2x
                        # the probability is the ratio of elements that the last filter contains that lie in the 2x range
                        prob_in_bounds = (sum(self.counters) - firsttwo_contain) / float(self.counters[last])
                        val = 1.0 * (1.0 - FPP_RATE) * prob_in_bounds
                        self.ghosthits += val
                else:
                    val = 1.0 * (1.0 - FPP_RATE)
                    self.ghosthits += val
                found = True
                break
            start += self.counters[index]
            #print "Miss but found key(%s) in cf #%d" % (key, index)
        if not found:
            self.ghostmisses += 1
        else:
            count = float(end - start)
            #TODO: use PLUS array
            for i in xrange(start, end):
                self.pdf[i] += val / count
        return found

    # eviction in the main cache
    # use: Evict(key)
    def Evict(self, key):
        if self.counters[self.first] >= self.capacity_per_filter:
            self.rotateFilters()
        if not self.cfs[self.first].check(key):
            #print "inserting %s into the first filter" % key
            self.cfs[self.first].add(key, 0)
            self.counters[self.first] += 1

    def rotateFilters(self):
        assert self.counters[self.first] >= self.capacity_per_filter
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

    def printStatistics(self):
        self.printCounters()
        print "ghosthits=%d" % self.ghosthits
        print "ghostmisses=%d" % self.ghostmisses
        print "rotations=%d" % self.rotations
