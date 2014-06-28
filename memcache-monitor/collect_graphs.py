import argparse
import datetime
import functools
import hashlib
import io
import os
import random
import re
import socket
import sys
import time
import yaml

from numpy import arange
import matplotlib.pyplot as plt
from flask import Flask, flash, g, redirect, abort, render_template, url_for as real_url_for, request, session, send_from_directory, send_file

from memcache import Client

mcClients = []
opts = dict()
app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///data.db'
app.secret_key = "10e37fa353c7454537kefjakdsc79d08f20f4623e247"

def url_for(path, **values):
    res = real_url_for(path, **values)
    return res

def make_cdf(pdfdict, clsid, requests):
    sm = 0.0
    cdf = []
    for i in xrange(100):
        val =  float(pdfdict[0][1]["pdf_%d_%d" % (clsid, i)])
        sm += val
        cdf.append( sm / float(requests))
    return cdf

def normalize_cdf(cdfdict, clsid, requests):
    cdf = []
    for i in xrange(100):
        val =  float(cdfdict[0][1]["cdf_%d_%d" % (clsid, i)])
        cdf.append( val / float(requests))
    return cdf

def make_pdfdict(plusdict, clsid):
    pdfdict = {}
    for i in xrange(100):
        s = "pdf_%d_%d" % (clsid, i)
        pdfdict[s] = 0.0
    for i in xrange(100):
        for j in xrange(i, 100):
            s = "plus_%d_%d" % (clsid, j)
            val =  float(plusdict[0][1][s])
            pdfdict[s.replace("plus", "pdf")] += val

    if int(clsid) == 6:
        print sorted(pdfdict.items())
    return [("127.0.0.1:11211", pdfdict)]


def make_graph(pdf):
    # Serve PNG made with matplotlib
    return

@app.after_request
def after_request(response):
    response.headers.add('Last-Modified', datetime.datetime.now())
    response.headers.add('Cache-Control', 'no-store, no-cache, must-revalidate, post-check=0, pre-check=0')
    response.headers.add('Pragma', 'no-cache')
    response.headers.add('Expires', '0')
    return response

@app.route('/')
def index():
    initclients() # todo: reconnect?
    # show all graphs
    data = {}
    return render_template('index.html', data=data)

@app.route('/gettotalroundercdf/')
def gettotalroundercdf():
    stats = []
    initclients() # todo: reconnect?
    fns = []
    for mc in mcClients:
        basic_stats = mc.get_stats()
        requests = int(basic_stats[0][1]["cmd_get"])
        total_mem = int(basic_stats[0][1]["limit_maxbytes"])
        total_mb = total_mem >> 20
        cdfdict = mc.get_stats("pluscdf")
        clsid = 1
        cdf = []
        for i in xrange(100):
            sm = 0.0
            for clsid in xrange(1,43):
                s = "cdf_%d_%d" % (clsid, i)
                sm += float(cdfdict[0][1][s])
            cdf.append(sm / float(requests))
        stats.append(cdf)
        plt.clf()
        fig, ax = plt.subplots()
        ax.set_xlabel("Cache size MB")
        ax.set_ylabel("Cumulative hit rate")
        ax.plot(range(100), cdf)
        ax.set_xlim(0, 100)
        maxy = max(cdf)
        if 1.5* maxy <= 1.0:
            ax.set_ylim(0, 1.5*maxy)
        else:
            ax.set_ylim(0, maxy)

        locs, labels = plt.xticks()

        # set the locations of the xticks

        div = 4
        new_labels = []
        new_ticks = []
        for i in xrange(0, div+1):
            mb = int( (total_mb * i ) / float(div) )
            xv = int( (100 * i) / float(div))
            new_ticks.append(xv)
            new_labels.append('%d MB' % mb)
        plt.xticks( new_ticks, new_labels )
        ax.grid(True)
        #fig.autofmt_xdate()
        fn = "img/totalcdf.png"
        plt.savefig("static/" + fn)
        fns.append(fn)
    plt.clf()
    return render_template('totalcdf.html', stats=stats, fns=fns)


@app.route('/getcdfs/')
def getcdfs():
    stats = []
    initclients() # todo: reconnect?
    fns = []
    d3cdfs = []
    for mc in mcClients:
        basic_stats = mc.get_stats()
        requests = int(basic_stats[0][1]["cmd_get"])
        cdfdict = mc.get_stats("cdf")
        # TODO make all cdfs
        clsid = 1
        while 1:
            test = "cdf_%d_0" % clsid
            if test not in cdfdict[0][1]:
                break
            cdf = normalize_cdf(cdfdict, clsid, requests)
            stats.append(cdf)
            plt.clf()
            fig, ax = plt.subplots()
            ax.plot(range(100), cdf)
            ax.set_xlim(0, 100)
            maxy = max(cdf)
            if 1.5* maxy <= 1.0:
                ax.set_ylim(0, 1.5*maxy)
            else:
                ax.set_ylim(0, maxy)
            ax.grid(True)
            fig.autofmt_xdate()
            fn = "img/slab_%d.png" % (clsid)
            plt.savefig("static/" + fn)
            fns.append(fn)
            clsid += 1
    plt.clf()
    return render_template('cdfs.html', stats=stats, fns=fns, d3cdfs=cd3cdfs)

@app.route('/getroundercdfs/')
def getroundercdfs():
    stats = []
    initclients() # todo: reconnect?
    fns = []
    for mc in mcClients:
        basic_stats = mc.get_stats()
        requests = int(basic_stats[0][1]["cmd_get"])
        cdfdict = mc.get_stats("pluscdf")
        clsid = 1
        while 1:
            test = "cdf_%d_0" % clsid
            if test not in cdfdict[0][1]:
                break
            cdf = normalize_cdf(cdfdict, clsid, requests)
            stats.append(cdf)
            plt.clf()
            fig, ax = plt.subplots()
            ax.plot(range(100), cdf)
            ax.set_xlim(0, 100)
            maxy = max(cdf)
            if 1.5* maxy <= 1.0:
                ax.set_ylim(0, 1.5*maxy)
            else:
                ax.set_ylim(0, maxy)
            ax.grid(True)
            fig.autofmt_xdate()
            fn = "img/slab_%d.png" % (clsid)
            plt.savefig("static/" + fn)
            fns.append(fn)
            clsid += 1
    plt.clf()
    return render_template('cdfs.html', stats=stats, fns=fns)

@app.route('/getroundercdfsd3/')
def getroundercdfsd3():
    stats = []
    initclients() # todo: reconnect?
    fns = []
    d3cdfs = []
    hits = []
    for mc in mcClients:
        basic_stats = mc.get_stats()
        requests = int(basic_stats[0][1]["cmd_get"]) # total number of requests
        slabdict = mc.get_stats("slabs")
        cdfdict = mc.get_stats("pluscdf")
        clsid = 1
        while 1:
            # how to get the number of requests for this slabclass ?
            test = "cdf_%d_0" % clsid
            if test not in cdfdict[0][1]:
                break
            try:
                get_hits = slabdict[0][1]['%d:get_hits' % clsid]
            except:
                get_hits = 0
            hits.append((clsid, float(get_hits) / float(requests), get_hits, requests))
            cdf = normalize_cdf(cdfdict, clsid, requests)
            d3cdf = [{'x': i, 'y' : cdf[i]} for i in xrange(100) ]
            d3cdfs.append(d3cdf)
            clsid += 1
    return render_template('cdfsd3.html',  d3cdfs=d3cdfs, hits=hits)


@app.route('/getplus/')
def getplus():
    # TODO: call memcached and get the plus arrays
    stats = []
    initclients() # todo: reconnect?
    for mc in mcClients:
        stats.append(mc.get_stats("plus"))
    return render_template('stats.html', stats=stats)


@app.route('/stats')
def stats():
    stats = []
    initclients() # todo: reconnect?
    for mc in mcClients:
        stats.append(mc.get_stats())
    return render_template('stats.html', stats=stats)

@app.route('/ghoststats')
def ghoststats():
    stats = []
    initclients() # todo: reconnect?
    s = ""
    for mc in mcClients:
        stats.append(mc.get_stats())
        slabdict = mc.get_stats("slabs")
        basic_stats = mc.get_stats()
        ghostplus =  mc.get_stats("ghostplus")
        sets = int(basic_stats[0][1]["cmd_set"]) # total number of requests
        gets = int(basic_stats[0][1]["cmd_get"]) # total number of requests
        for k in slabdict[0][1].keys():
            if k.find(":") == -1:
                continue
            num, cmd = k.split(":")
            if cmd != "get_hits":
                continue
            clsid = int(num)
            hits = int(slabdict[0][1]['%d:get_hits' % clsid])
            ghits = float(ghostplus[0][1]["ghostcdf_0_0"])
            s += " ===== class id: %d =====\n" % clsid
            s +=  "%d sets, %d gets\n" % (sets, gets)
            s += "%d hits, %.2f ghosthits\n" % (hits, ghits)
            try:
                hitratio = hits / float(gets)
                ghitratio = (ghits + hits) / float(gets)
                s += "hitratio: %.5f , hitratio_using_ghosts: %.5f\n" % (hitratio, ghitratio)
            except:
                s += "whooops no gets yet :P\n\n"
    return render_template('ghoststats.html', stats=stats, s=s)


@app.route('/slabstats')
def slabstats():
    stats = []
    initclients() # todo: reconnect?
    for mc in mcClients:
        stats.append(mc.get_stats("slabs"))
    return render_template('stats.html', stats=stats)

@app.route('/itemstats')
def itemstats():
    stats = []
    initclients() # todo: reconnect?
    for mc in mcClients:
        stats.append(mc.get_slabs())
    return render_template('stats.html', stats=stats)


@app.context_processor
def context_processor():
    return dict(
        app=app,
        current_user="trausti",
        url_for=url_for,
        r=random.randint(0, 1000), # random integer
        mcClients=mcClients,
    )

@app.before_request
def before_request():
    g.request_start_time = time.time()
    g.request_time = lambda: time.time() - g.request_start_time


def initclients():
    global mcClients, serverList, opts
    serverList = [["%s:%s" % (opts.memcachedhost, opts.memcachedport)]]
    mcClients = []
    for servers in serverList:
        mc = Client(servers, debug=2)
        mcClients.append(mc)
        print "Initializing memcachedclient for: %s => %s"  % (servers, mc)

def main(argv):
    global app, opts, conn, mc, serverList, mcClients
    parser = argparse.ArgumentParser(description='Memcached slab monitoring framework.')
    parser.add_argument('-p', '--port', default=6117, type=int, help='the port to listen on')
    parser.add_argument('-H', '--host', default='', help='the host to listen on')
    parser.add_argument('-m', '--memcachedhost', default='127.0.0.1', help='the host to collect statistics from')
    parser.add_argument('-o', '--memcachedport', default='11211', help='the memcached port to get statistics from')
    parser.add_argument('-d', '--debug', default=False, action='store_true', help='run in debug mode')
    opts = parser.parse_args(argv)
    #db.init_app(app)

    initclients() # todo: reconnect?

    app.run(host=opts.host, port=opts.port, debug=opts.debug)


if __name__ == '__main__':
    main(sys.argv[1:])
