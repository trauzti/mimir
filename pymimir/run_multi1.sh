#!/bin/sh
algs="RANDOM LRU CLOCK LFU LRU3 ARC"
cstart="100"
cincr="100"
cend="4000"

for trc in $(ls /huge/Research/Jiang_traces/lirs-trc/multi1*)
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
