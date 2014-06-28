import unittest
import random
import sys

sys.path.append("../")
from cache import Cache
from common import Entry, make_cdf

import ARC, CLOCK, LRU, LFU, LRU3, LRU10
CACHESIZE = 500


class TestStatistics(unittest.TestCase):
    def setUp(self):
        pass

    def test_algorithm(self, name=None):
        if name == None:
            return
        self.cache = Cache(name, CACHESIZE)

        for j in xrange(CACHESIZE):
            key = str(j)
            self.cache.put(key, "A")

        for j in xrange(CACHESIZE/2):
            key = str(j)
            self.cache.get(key)
        self.assertEqual(self.cache.cache.stats.stats.hits, CACHESIZE/2)
        self.assertEqual(self.cache.cache.stats.stats.requests, CACHESIZE/2)

        for j in xrange(CACHESIZE/2):
            key = str(j)
            self.cache.get(key)
        self.assertEqual(self.cache.cache.stats.stats.hits, CACHESIZE)
        self.assertEqual(self.cache.cache.stats.stats.requests, CACHESIZE)

        for j in xrange(CACHESIZE, 2*CACHESIZE):
            key = str(j)
            self.cache.get(key)
        hits = self.cache.cache.stats.stats.hits
        self.assertEqual(hits, CACHESIZE)
        misses = self.cache.cache.stats.stats.misses
        self.assertEqual(misses, CACHESIZE)
        requests = self.cache.cache.stats.stats.requests
        self.assertEqual(requests, 2*CACHESIZE)

        self.cache.cache.stats.stats.make_pdf()
        self.cache.cache.stats.ghostlist.make_pdf()
        self.pdf = self.cache.cache.stats.stats.pdf
        self.gpdf = self.cache.cache.stats.ghostlist.pdf
        self.cdf = make_cdf(self.pdf, CACHESIZE, 1)
        self.gcdf = make_cdf(self.gpdf, CACHESIZE, 1) # The default size of the ghostlist is the same as the cache
        self.assertTrue((self.cdf[CACHESIZE] - hits) < 1e-5) # the number of hits
        self.assertEqual(self.gcdf[0], 0) # no extra hits in the ghostlist!
        self.assertEqual(self.gcdf[CACHESIZE-1], 0) # no extra hits in the ghostlist!


    def test_LRU(self):
        self.test_algorithm("LRU")

    def test_CLOCK(self):
        self.test_algorithm("CLOCK")

    def test_LFU(self):
        self.test_algorithm("LFU")

    def test_LRU3(self):
        self.test_algorithm("LRU3")

    """
    def test_LRU10(self):
        self.test_algorithm("LRU10")
    """

    """
    # TODO: fix ARC
    def test_ARC(self):
        self.test_algorithm("ARC")
    """


if __name__ == '__main__':
    unittest.main()
