import os
import re
import sys

import numpy as np
import matplotlib.pyplot as plt

#import numpy as np

class DictWrapper(dict):
     def __getattr__(self, name):
         if name in self:
             return self[name]
         else:
             raise AttributeError(name)

files = os.listdir("output")
algs = {}

class Organizer(dict):
    files = [] # DictWrapper, content),... ,
    def getall(self, r=None, mem=None, alg=None, cachesize=None):
        ret = []
        for dw in self.files:
            use = True
            if r and dw.r != r:
                use = False
            if alg and dw.alg != alg:
                use = False
            if use:
                ret.append(dw)
        return ret


starttimes = {} # (alg, bc, r, mem, threads)
endtimes = {} # (alg, bc, r, mem, threads)


o = Organizer()
for fn in files:
    l = open("output/%s" % fn, "r").readlines()
    fm = re.match("lru_(?P<alg>\w+)_(?P<r>\d+)", fn)
    d1 = DictWrapper(fm.groupdict())
    d2 = None
    for line in l:
        m1 = re.match("Running time: (?P<t>\d+.\d+)", line)
        m2 = re.match("Throughput: (?P<tp>\d+.\d+) OPS/s", line)
        if m1:
            d2 = DictWrapper(m1.groupdict())
        elif m2:
            d2 = DictWrapper(m2.groupdict())
        else:
            continue
    d1.update(d2)
    algs[d1.alg] = 1
    o.files.append(d1)

averages = {}
stdevs = {}
alg_order = [("lru", "regular"), ("lru+rounder","rounder"), ("lru+avl","avl"), ("lru+mattson","mattson")]
#alg_order = [("lru", "regular"), ("lru+avl","avl"), ("lru+mattson","mattson")]
sizes = [5000]
N = len(alg_order)
M = len(sizes)
print algs
for n,alg in alg_order:
    for sz in sizes:
        ores =  o.getall(alg=alg)
        arr = [float(x.tp) for x in ores]
        avg = sum(arr) / float(len(arr))
        stdev = np.std(arr)
        print alg, avg, stdev
        averages[alg,sz] = avg
        stdevs[alg,sz] = stdev

ind = np.arange(M)  # the x locations for the groups
width = 0.14       # the width of the bars
offset = 0.10

#print o.files
fig, ax = plt.subplots()
rects = []
i = 0
colors = ['m', 'y', 'g', 'b', 'c']
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
ax.set_ylabel('Throughput [OPS/s]')
#ax.set_xlabel('Cache size [#items]')
ax.set_title('Statistics algorithms')
ax.set_xticks(ind+offset + width *(M / 2.0))
#ax.set_xticklabels( tuple(map(str, sorted(mems.keys()))) )

ax.legend( tuple(rects[i][0] for i in xrange(N)), tuple(rects[i][1] for i in xrange(N)) )
#ax.legend( (rects1[0], rects2[0]), ('Unmodified', 'Rounder B=4') )

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

plt.savefig('overhead.png', format='png')
plt.savefig('overhead.eps', format='eps', dpi=1000)
plt.show()
