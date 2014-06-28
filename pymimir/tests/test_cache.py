import socket
import time
import unittest
import sys
sys.path.append("../")

from cache import Cache, MemcachedFactory
from common import Entry
from threading import Thread

class MemcachedFactoryThread(Thread):
    def __init__(self):
        Thread.__init__(self)
        self.cache = Cache("LRU", 10)
        self.server = MemcachedFactory("", 1337, self.cache)
    def run(self):
        self.server.start(installSignalHandlers=False)


class TestCache(unittest.TestCase):
    def setUp(self):
        self.t = MemcachedFactoryThread()
        self.t.start()

    def test_socket(self):
        #TODO: move this to a client
        time.sleep(3)
        s = socket.socket()
        address = '127.0.0.1'
        port = 1337 # port number is a number, not string
        try:
            s.connect((address, port))
        except Exception, e:
            print "Failed to connect"
            raise e
        s.send("get trausti\r\n")
        data = s.recv(1024)
        self.assertEqual(data, "END\r\n")
        s.send("set trausti 0 0 4\r\nasdf\r\n")
        data = s.recv(1024)
        self.assertEqual(data, "STORED\r\n")
        s.send("get trausti\r\n")
        data = s.recv(1024)
        print data
        self.assertEqual(data, "VALUE trausti 0 4\r\nasdf\r\nEND\r\n")
        s.send("kill\r\n")
        #data = s.recv(1024)
        #self.assertEqual(data, "100")

if __name__ == '__main__':
    unittest.main()
