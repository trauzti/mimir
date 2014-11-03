#!/usr/bin/python2.7
import os
import sys
import time

from collections import defaultdict

from common import Entry, statbase, make_cdf


class alg:
    def __init__(self, c, *args, **kwargs):
        self.l = []
        self.cachesize = c
        self.requestcount = 0
        self.hitarray = [0 for i in xrange(c)]
        self.mattson = True

    def writeStatistics(self):
        print "Writing the CDF to a file"
        self.cdf = make_cdf(self.hitarray, self.cachesize, self.requestcount)
        with open("cdfs/mattson_size=%d.txt" % self.cachesize, "w") as f:
            f.write('\n'.join(map(str, self.cdf)) + '\n')

    def __repr__(self):
        return "LRU(stacksize=%d)" % self.cachesize

    def get(self, key):
        self.requestcount += 1
        e = None
        for i in xrange(len(self.l)):
            if self.l[i].key == key:
                e = self.l[i]
                break
        if e:
            self.hitarray[i] += 1
            self.l = [e] + self.l[:i] + self.l[i+1:] # Put the key in front
            # This still works if i == 0 !!
            return key
        else:
            return None

    def put(self, key, value=None):
        e = Entry(key)
        for i in xrange(len(self.l)):
            if self.l[i].key == key:
                e = self.l[i]
                return 1

        if len(self.l) == self.cachesize:
            ed = self.l[-1]
            self.l = [e] + self.l[:self.cachesize-1]  # Put they key in front and remove the last
        else:
            self.l = [e] + self.l[:self.cachesize]   # Don't remove the last!!
        return 1
