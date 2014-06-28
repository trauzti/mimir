#!/usr/bin/python2
import os
import re
import sys
import argparse

import matplotlib.pyplot as plt
from pylab import *

"""


opens output files from ./output
creates graphs from the output files in ./images

to create output files in ./output
$ ./run_alot.sh


"""
def makediffs(X, Y, hX, hY):
    diffs = []
    for i in xrange(len(X)):
        x = X[i]
        y = Y[i]
        delta = abs(y - hY[x])
        diffs.append(delta)
    return diffs

def calculateMAE(diffs):
    N = len(diffs)
    return sum(diffs) / float(N)

class DictWrapper(dict):
     def __getattr__(self, name):
         if name in self:
             return self[name]
         else:
             raise AttributeError(name)

class Organizer(dict):
    files = [] # DictWrapper, content),... ,
    def getall(self, bc=None, trc=None, alg=None, csize=None, gsize=None):
        ret = []
        for dw in self.files:
            use = True
            if bc and dw.bc != bc:
                use = False
            if alg and dw.alg != alg:
                use = False
            if trc and dw.trc != trc:
                use = False
            if csize and dw.csize != csize:
                use = False
            if gsize and dw.gsize != gsize:
                use = False
            if use:
                ret.append(dw)
        return ret

def main(BASESIZE, outputdir):
    files = os.listdir(outputdir)
    algs = {}
    traces = {}
    o = Organizer()
    # DOING: change this to open all the files in a specific directory and plot the histograms together
    for fn in files:
        lines = open("%s/%s" % (outputdir, fn), "r").readlines()
        # filename: algorithm_trace_buckets_cachesize_ghostlistsize.(histogram|stats)
        fm = re.match("(?P<alg>\w+)_(?P<trc>\w+)_(?P<bc>\d+)_(?P<csize>\d+)_(?P<gsize>\d+)\.(?P<type>\w+)", fn)
        d1 = DictWrapper(fm.groupdict())
        if d1.type == "stats":
            m1 = re.match("Hits=(?P<hits>\d+), Misses=(?P<misses>\d+), Requests=(?P<requests>\d+)", lines[0])
            d2 = DictWrapper(m1.groupdict())
            d1.update(d2)
            algs[d1.alg] = 1
            traces[d1.trc] = 1
            histogramlines = open("%s/%s" % (outputdir, fn.replace("stats", "histogram")), "r").readlines()
            d1.histogram = [(int(x[0]), float(x[1])) for x in map(lambda x:x.split(" "), histogramlines)]
            o.files.append(d1)
        elif d1.type == "histogram":
            pass
        else:
            assert 0
    print len(o.files)

    #BASESIZE = 1500
    for alg in algs.keys():
        for trc in traces.keys():
            print "Generating plot for %s,%s" % (alg, trc)
            plt.clf()
            fig, ax = plt.subplots()
            ores = o.getall(alg=alg, trc=trc)
            ores.sort(key=lambda x: int(x.csize))
            X = [int(x.csize) for x in ores]
            Y = [float(x.hits) / float(x.requests) for x in ores]
            histogram = filter(lambda x:int(x.csize) == BASESIZE, ores)[0].histogram
            hX = [z[0] for z in histogram]
            hY = [z[1] for z in histogram]
            diffs = makediffs(X, Y, hX, hY)
            #print diffs
            print "maxdiff = %.1f%%, mindiff = %.1%%f, MAE=%.1f%%\n" % (100 * max(diffs), 100 * min(diffs), 100 * calculateMAE(diffs))
            #print alg, [(x.csize, x.hits) for x in ores]
            plt.plot(X, Y, 'ro')
            plt.plot(hX, hY)
            fpref = "images/%s_%s_%d" % (alg, trc, BASESIZE)
            ax.set_ylabel('Cumulative hit rate')
            ax.set_xlabel('Cache size (# items)')
            ax.set_title('Algorithm: %s, trace: %s and base from %d items ' % (alg, trc, BASESIZE))
            plt.savefig('%s.png' % fpref, format='png')
            plt.savefig('%s.eps' % fpref, format='eps', dpi=1000)


    """
    averages = {}
    stdevs = {}
    alg_order = [("lru", "regular"), ("lru+rounder","rounder"), ("lru+avl","avl"), ("lru+mattson","mattson")]
    sizes = [5000]
    N = len(algs.keys())
    M = len(sizes)
    for alg in algs.keys():
        for sz in sizes:
            ores =  o.getall(alg=alg)
            arr = [float(x.t) for x in ores]
            avg = sum(arr) / float(len(arr))
            stdev = np.std(arr)
            print alg, avg, stdev
            averages[alg,sz] = avg
            stdevs[alg,sz] = stdev

    ind = np.arange(M)  # the x locations for the groups
    width = 0.14       # the width of the bars
    offset = 0.10

    print o.files
    fig, ax = plt.subplots()
    rects = []
    i = 0
    colors = ['y', 'g', 'b', 'c', 'm', 'p']
    for n, alg in alg_order:
        rMeans = tuple( averages[alg,size]  for size in sizes )
        rStd = tuple( stdevs[alg,size]  for size in sizes )
        rBar = ax.bar(ind + offset + width*(i+1), rMeans, width, color=colors[i], yerr=rStd)
        rects.append((rBar, "%s" % n))
        i += 1

    #womenMeans = tuple( averages[mem, 'rounder', 4]  for mem in sorted(mems.keys()))
    #womenStd =   (3, 5, 2, 3, 3)
    #rects2 = ax.bar(ind+width, womenMeans, width, color='y', yerr=womenStd)
    #rects2 = ax.bar(ind+width, womenMeans, width, color='y')

    # add some
    ax.set_ylabel('Running time [s]')
    ax.set_xlabel('Cache size')
    ax.set_title('Statistics algorithms')
    ax.set_xticks(ind+offset + width *(M / 2.0))
    #ax.set_xticklabels( tuple(map(str, sorted(mems.keys()))) )

    ax.legend( tuple(rects[i][0] for i in xrange(N)), tuple(rects[i][1] for i in xrange(N)) )
    #ax.legend( (rects1[0], rects2[0]), ('Unmodified', 'Rounder B=4') )
    """

    """ # TODO: fixme!!
    memskeys = sorted(mems.keys())
    def autolabel(_rects, mc):
        # attach some text labels
        i = 0
        for _rect in _rects:
            ovhead = overheads[memskeys[i], bc]
            height = _rect.get_height()
            ax.text(_rect.get_x()+_rect.get_width()/2., 1.05*height, '%.1f%%' % ovhead,
                    ha='center', va='bottom', fontsize = 4)
            i += 1

    #autolabel(rects1)
    i = 1
    for bc in sorted(bcs.keys()):
        autolabel(rects[i][0], bc)
        i += 1
    """

    #plt.savefig('overhead.png', format='png')
    #plt.savefig('overhead.eps', format='eps', dpi=1000)
    #plt.show()
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", "--outputdir", help="The output directory to use", type=str, default="", required=True)
    parser.add_argument("-b", "--basesize", help="The basesize to use", type=int, default=700, required=True)
    args = parser.parse_args()
    outputdir = args.outputdir
    BASESIZE = args.basesize
    main(BASESIZE, outputdir)
