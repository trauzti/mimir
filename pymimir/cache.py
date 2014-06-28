#!/usr/bin/python2
import argparse
import asyncore
import re
import socket
import os
import sys
import errno
import time

verbose = False
verbose = True

from twisted.internet import reactor, protocol

import ARC, CLOCK, LRU, LFU, LRU3, LRU10, RANDOM

from statistics import statscollector

cache = None
cache_algorithms = {
    "ARC": ARC.alg,
    "CLOCK": CLOCK.alg,
    "LRU": LRU.alg,
    "LRU3": LRU3.alg,
    "LRU10": LRU10.alg,
    "LFU": LFU.alg,
    "RANDOM": RANDOM.alg,
}


class MemCache(protocol.Protocol):
    def __init__(self, *args, **kwargs):
        self.factory = kwargs["factory"]
        del kwargs["factory"]
        self.cache = self.factory.cache
        self.clientdata = {}

    def connectionMade(self):
        peer = self.transport.getPeer()
        hp = (peer.host, peer.port)
        print 'Incoming connection from %s:%s' % hp

    def handle_get(self, header):
        # get <key>*\r\n
        l = header.split(" ")
        if verbose:
            print "get %s" % l
        key = l[1]
        flags = 0 # 32 byte int # TODO: retrieve a saved flag
        value = self.cache.get(key)
        tosend = ""
        if value:
            tosend += "VALUE %s %d %d\r\n%s\r\n" % (key, flags, len(value), value)
        tosend += "END"
        if verbose:
            print "GET returning %s" % (True if value else False)
        self.transport.write("%s\r\n" % tosend)

    def handle_set(self, header, value):
        #set <key> <flags> <exptime> <bytes> [noreply]\r\n<value>\r\n#
        peer = self.transport.getPeer()
        hp = (peer.host, peer.port)
        l = header.split(" ")
        if verbose:
            print "set %s" % l
        cmd, key, flags, exptime, bytes = l
        assert len(value) == int(bytes)
        putresult = self.cache.put(key, value)
        tosend = ""
        if putresult == 1:
            tosend += "STORED"
        elif putresult == 0:
            tosend += "SERVER_ERROR"
        if verbose:
            print "SET returning"
        self.transport.write("%s\r\n" % tosend)

    def handle_data(self, data):
        all_data = data.split("\r\n")
        peer = self.transport.getPeer()
        hp = (peer.host, peer.port)
        # get read until \r\n
        # set read all the bytes
        i = 0
        while i < len(data):
            i = data.find("\r\n")
            if i == -1:
                if verbose:
                    print "need more data, saving data for later"
                self.clientdata[hp] = data
                return
            if not data:
                assert i == -1
                return
            if data[:3] == "get":
                self.handle_get(data[:i])
                data = data[i+2:]
            elif data[:3] == "set":
                self.last = data
                if data.count("\r\n") < 2:
                    if verbose:
                        print "need more data, saving data for later"
                    self.clientdata[hp] = data
                    return
                header = data[:i]
                rest = data[i+2:]
                cmd, key, flags, exptime, bytes = header.split(" ")
                bytes = int(bytes)
                value = rest[:bytes]
                assert len(value) == bytes
                assert rest[bytes:bytes+2] == "\r\n"
                leftover = rest[bytes+2:]
                self.handle_set(header, value)
                self.clientdata[hp] = leftover
                data = leftover
            elif data[:9] == "stats hrc":
                cdf = self.cache.stats.stats.cdf
                gcdf = self.cache.stats.stats.cdf
                # TODO: send the hrc
                self.transport.write("hrc\r\n")
                break
            elif data[:4] == "quit":
                print "Client quit"
                self.transport.loseConnection()
                break
            elif data[:4] == "kill":
                print "killing server"
                reactor.stop()
                break
            else:
                print [data[:100]]
                print "ERROR"
                from ipdb import set_trace; set_trace()
                # this fails with mutilate
                self.transport.write("ERROR\r\n")


    def dataReceived(self, data):
        if verbose:
            print "dataReceived"
        peer = self.transport.getPeer()
        hp = (peer.host, peer.port)
        olddata = self.clientdata.get(hp)
        if olddata:
            data = olddata + data
            del self.clientdata[hp]
        # memcached telnet example
        # get trausti
        # END
        # set trausti 0 900 4
        # asdf
        # STORED
        # get trausti
        # VALUE trausti 0 4
        # asdf
        # END
        if not data:
            print "no data"
            return
        self.handle_data(data)

class MemcachedFactory(protocol.Factory):
    protocol = MemCache

    def __init__(self, host, port, cache):
        self.cache = cache
        reactor.listenTCP(port ,self)
        #protocol.Factory.__init__()

    def buildProtocol(self, addr):
        return self.protocol(factory=self)

    def start(self, installSignalHandlers=True):
        reactor.run(installSignalHandlers=installSignalHandlers)

class Cache(object):
    def __init__(self, name, size, bc=64, filters=3):
        if name not in cache_algorithms.keys():
            raise Exception("%s not valid" % name)
        self.name = name
        self.bc = bc
        self.filters = filters
        self.size = size
        self.cache = cache_algorithms[name](size)
        self.cache.stats = statscollector(size=size, bc=bc, filters=filters)
        print "Cache initialized. Algorithm=%s, size=%d" % (name, size)
    def put(self, *args, **kwargs):
        return self.cache.put(*args, **kwargs)
    def get(self, *args, **kwargs):
        return self.cache.get(*args, **kwargs)

    # make it resizable

    def stdin(self):
        # read from stdin
        self.put("trausti", "awesome")
        print self.get("trausti")
        while 1:
            r = raw_input()
            print r
            l = r.split(" ")
            print l
            if l[0] == "get":
                print self.get(l[1])
            elif l[0] == "set":
                print self.put(l[1], l[2])
            elif l[0] == "kill":
                print "stopping server"
                break


    def file(self, filename):
        for line in open(filename, "r").xreadlines():
            l = line.split(" ")
            if not self.get(l[0]):
                self.put(l[0], l[0])
        self.cache.stats.collectStatistics()
        jcdf = self.cache.stats.jcdf

        # filename: algorithm_trace_buckets_cachesize_ghostlistsize
        trcbase = os.path.split(filename)[-1].replace(".trc", "")
        trcbase = trcbase.replace("_", "")
        fprefix = "output/%s_%s_%d_%d_%d" % (self.name,trcbase, self.bc, self.size, self.size)
        self.cache.stats.printStatistics("%s.stats" % fprefix)
        f = open("%s.histogram" % fprefix , "w")
        for i in xrange(len(jcdf)):
            f.write("%d %f\n" % (i, jcdf[i]))
        f.close()


### The telnet interface ###
class TelnetCommandInterface(object):
    def __init__(self):
        pass


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-H", "--host", help="The address to listen on", type=str, default="", required=False)
    parser.add_argument("-P", "--port", help="The port to listen on", type=int, default=1337, required=False)
    parser.add_argument("-c", "--cachealgorithm", help="The cache algorithm to use", type=str, required=True)
    parser.add_argument("-n", "--cachesize", help="The cache size to use", type=int, required=True)
    parser.add_argument("-l", "--listen", help="Listen over network", type=bool, default=True, required=False)
    parser.add_argument("-t", "--tracefile", help="Use this trace file", type=str, default="", required=False)
    args = parser.parse_args()

    cache = Cache(args.cachealgorithm, args.cachesize)
    if args.tracefile:
        print "Reading requests from file: %s" % args.tracefile
        cache.file(args.tracefile)
    elif args.listen:
        print "Listening on port %d" % args.port
        m = MemcachedFactory(args.host, args.port, cache)
        m.start()
    else:
        print "Reading requests from stdin"
        cache.stdin()
