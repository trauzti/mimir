import unittest
import random
import sys

from numpy.random import zipf

sys.path.append("../")
import ARC, CLOCK, LRU, LFU, LRU3, LRU10
from cache import Cache
from common import Entry


CACHESIZE = 500
NUMREQUESTS = 10000
key_alpha = 1.33
keydistribution = zipf(key_alpha, NUMREQUESTS)

class TestAlgorithms(unittest.TestCase):
    def setUp(self):
        pass

    def test_algorithm(self, name=None):
        if name == None:
            return
        self.cache = Cache(name, CACHESIZE)
        self.assertEqual(str(self.cache.cache), name)
        self.assertEqual(self.cache.get("trausti"), None)
        self.assertEqual(self.cache.put("trausti", 100), 1)
        self.assertEqual(self.cache.get("trausti"), 100)

        for j in xrange(2000):
            self.assertEqual(self.cache.put("%d" % j, 200), 1)

        self.assertEqual(self.cache.get("1999"), 200)

        for j in xrange(2000, 3000):
            self.assertEqual(self.cache.get("%d" % j), None)

        for j in xrange(NUMREQUESTS):
            key = str(keydistribution[j])
            if not self.cache.get(key):
                self.cache.put(key, "A")

    def test_LRU(self):
        self.test_algorithm("LRU")
        self.cache.cache.walk()
        self.cache.put("hestur", 100)
        d = self.cache.cache.get_stackdistance("hestur")
        self.assertEqual(d[0], 0)
        self.assertEqual(d[1], True)
        self.cache.put("skinka", 101)
        d = self.cache.cache.get_stackdistance("hestur")
        self.assertEqual(d[0], 1)
        self.assertEqual(d[1], True)

    def test_CLOCK(self):
        self.test_algorithm("CLOCK")

    def test_LFU(self):
        self.test_algorithm("LFU")

    def test_LRU3(self):
        self.test_algorithm("LRU3")
    """
    def test_LRU10(self):
        self.test_algorithm("LRU10")
    def test_ARC(self):
        self.test_algorithm("ARC")
    """


if __name__ == '__main__':
    unittest.main()
