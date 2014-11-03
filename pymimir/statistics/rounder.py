#!/usr/bin/python2.7

import os
import sys
import time

import smhasher

from common import statbase

# Should we count the first request to a key as a request?
# If we just count the second time, we can get 100% hitrate, otherwise not
# Currently: Start counting on the second request

class statclass(statbase):
    def __init__(self, c, bc, *args, **kwargs):
        R = kwargs.get("R", 1)
        statbase.__init__(self)
        self.cachesize = c
        self.bc = bc
        self.R = R
        self.RLIMIT = 10000
        if self.R == 1:
            self.RLIMIT = 1
        self.passfraction = self.R / float(self.RLIMIT)
        self.multiplier = float(self.RLIMIT) / self.R
        assert int(self.multiplier) == self.RLIMIT / self.R
        self.Rignored = 0
        self.Rpassed = 0
        self.MAX_ACTIVITY = bc - 1
        self.BOTTOM_VAL = 0
        self.TOP_VAL = self.MAX_ACTIVITY
        self.pdf = { i : 0.0 for i in xrange(2*c+1) }
        self.activecount = [0 for i in xrange(self.MAX_ACTIVITY+1)] # activecount[i] is the number of elements with i bits on
        self.cdf = None
        self.binsize = (c + 0.0) / bc
        self.maxinbin = 0
        self.binsizesum = 0

    def printSamplingStats(self):
        print "Sampling percentage stats"
        print "Wanted: %d/%d=%f" % (self.R, self.RLIMIT, self.R / float(self.RLIMIT))
        print "Got: %d/%d=%f" % (self.Rpassed, self.Rpassed + self.Rignored, self.Rpassed / (self.Rpassed + self.Rignored + 0.0))

    def __repr__(self):
        return "ROUNDER(stacksize=%d,bc=%d,binsize=%d)" % (self.cachesize, self.bc, self.binsize)

    ### Returns (start,end), element with activity
    ### a is located at stackdistance [start,...,end-1]
    def get_stackdistance(self, a):
        assert 0 <= a and a <= self.MAX_ACTIVITY
        start = 0
        i = self.MAX_ACTIVITY
        while 1:
            if i == a:
                break
            start += self.multiplier * self.activecount[i]
            i -= 1
        end = start + self.multiplier * self.activecount[a]
        if end > self.cachesize:
            print "start=%d,end=%dsum=%d,size=%d,R=%d,%s" % (start,end,sum(self.activecount), self.cachesize, self.R, self.activecount)
        # TODO: perhaps use floor or ceil
        # Fix start being bigger than cachesize :I:I
        start, end = int(start), int(min(end, self.cachesize))
        assert start <= self.cachesize
        assert end <= self.cachesize
        if not (start < end):
            print start, end, a
            print [ (i, self.activecount[i]) for i in xrange(self.MAX_ACTIVITY+1) ]
        assert start < end
        return start, end

    def map_activity(self, a):
        assert a <= self.TOP_VAL
        if a < self.BOTTOM_VAL:
            return 0
        return a - self.BOTTOM_VAL


    def update_pdf(self, a):
        assert 0 <= a and a <= self.MAX_ACTIVITY
        start, end = self.get_stackdistance(a)
        count = float(end-start)
        val = self.multiplier * 1.0 / count
        for i in xrange(start, end):
            self.pdf[i] += val

    # Call me when the first bin is full ...
    def ageentries(self):
        #print self.activecount
        #self.maxinbin = max(self.maxinbin, max(self.activecount))
        oldsm = sum(self.activecount)
        self.activecount[0] += self.activecount[1]
        i = 1
        while i < self.MAX_ACTIVITY:
            self.activecount[i] = self.activecount[i+1]
            i += 1
        self.activecount[self.MAX_ACTIVITY] = 0
        newsm = sum(self.activecount)
        assert oldsm == newsm
        assert newsm <= self.cachesize
        self.BOTTOM_VAL += 1
        self.TOP_VAL += 1
        #sys.stderr.write("Roundclock aging, BOTTOM,TOP=(%d,%d)\n" % (self.BOTTOM_VAL, self.TOP_VAL))
        assert self.TOP_VAL - self.BOTTOM_VAL == self.MAX_ACTIVITY

    def Miss(self, key):
        self.misses += 1
        self.requests += 1

    def Hit(self, e):
        self.hits += 1
        self.requests += 1
        key = e.key
        hv = smhasher.murmur3_x64_64(key)
        if (hv % self.RLIMIT) <  self.R:
            #print "Hit passed"
            self.Rpassed += 1
        else:
            #print "Hit ignored"
            self.Rignored += 1
            return
        ma = self.map_activity(e.activity)
        self.binsizesum += self.activecount[ma]
        self.update_pdf(ma)  # We want to update the PDF before changing any activitycount
        if self.R == 1:
            assert self.activecount[ma] >= 1
        self.activecount[ma] -= 1
        add = 0
        if ma == self.MAX_ACTIVITY:
            add = 1
        if self.activecount[-1] + add >= self.binsize:
            self.ageentries()
        self.activecount[self.MAX_ACTIVITY] += 1
        e.activity = self.TOP_VAL
        if self.R == 1:
            assert abs(self.hits - sum(self.pdf.values())) < 1.0e-5

    def Set(self, e):
        key = e.key
        hv = smhasher.murmur3_x64_64(key)
        if (hv % self.RLIMIT) <  self.R:
            #print "Set passed"
            self.Rpassed += 1
        else:
            #print "Set ignore"
            #self.Rignored += 1
            return
        e.activity = self.TOP_VAL
        self.activecount[self.MAX_ACTIVITY] += 1

    def Evict(self, e):
        key = e.key
        hv = smhasher.murmur3_x64_64(key)
        if (hv % self.RLIMIT) <  self.R:
            self.Rpassed += 1
        else:
            #print "Evict ignore"
            self.Rignored += 1
            return
        ma = self.map_activity(e.activity)
        self.activecount[ma] -= 1
        if self.R == 1:
            assert self.activecount[ma] >= 0
