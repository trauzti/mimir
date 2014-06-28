#!/bin/sh
algs="RANDOM LRU CLOCK LFU LRU3 ARC"
cstart="250"
cincr="250"
cend="3000"

prefix=/huge/Research/Jiang_traces/lirs-trc/
#cs.trc -c 1400
#gli.trc -c 2500
#ps.trc -c 3000
#cpp.trc -c 900
#2_pools.trc -c 450
#sprite.trc  -c 1000
#multi1.trc -c 2000
#multi2.trc -c 3000
#multi3.trc -c 4000

for trc in $(ls /huge/Research/Jiang_traces/lirs-trc/*)
do
    for alg in $algs
    do
        for c in $(seq $cstart $cincr $cend)
        do 
            echo  $trc $alg $c
            python2 cache.py -c $alg -n $c -t $trc
        done
    done

done


