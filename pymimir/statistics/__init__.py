import sys
sys.path.append("../")

import countingghost
import rounder



from common import make_cdf

class statscollector(object):
    def __init__(self, size=0, bc=0, filters=3, R=1):
        self.size = size
        self.bc = bc
        self.R = R
        self.filters = filters
        self.stats = rounder.statclass(size, bc)
        # default 3 filters in the countingghost class
        self.ghostlist = countingghost.ghostclass(capacity=size, R=R)

    def Miss(self, key):
        self.stats.Miss(key)
        self.ghostlist.Miss(key)

    def Hit(self, e):
        self.stats.Hit(e)

    def Set(self, e):
        self.stats.Set(e)

    def Evict(self, e):
        self.stats.Evict(e)
        self.ghostlist.Evict(e.key)

    # must call collectStatistics first!!
    def printStatistics(self, filename=None):
        size = self.size
        s = ""
        s += "Hits=%d, Misses=%d, Requests=%d, R=%d\n" % (self.stats.hits, self.stats.misses, self.stats.requests, self.R)
        s += "CDF here meaning, cache of size 1 etc (not stack distances)\n"
        s += "cdf[1]=%.5f cdf[%d]=%.5f\n" % (self.cdf[1], size, self.cdf[size])
        s += "gcdf[1]=%.5f gcdf[%d]=%.5f\n" % (self.gcdf[1], size, self.gcdf[size])
        s += "jcdf[1]=%.5f jcdf[%d]=%.5f\n" % (self.jcdf[1], 2*size, self.jcdf[2*size])
        if filename:
            f = open(filename, "w")
            f.write(s)
            f.close()
        else:
            print s

    def collectStatistics(self):
        size = self.size
        requests = self.stats.requests
        self.stats.make_pdf()
        self.ghostlist.make_pdf()
        self.jpdf = [ self.stats.pdf[i] for i in xrange(size) ] + [ self.ghostlist.pdf[i] for i in xrange(size) ]
        self.cdf = make_cdf(self.stats.pdf, size, requests)
        self.gcdf = make_cdf(self.ghostlist.pdf, size, requests) # The default size of the ghostlist is the same as the cache
        self.jcdf = make_cdf(self.jpdf, size * 2, requests)
