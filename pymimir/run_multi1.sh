#!/bin/sh
#algs="RANDOM LRU CLOCK LFU LRU3 ARC"
algs="LRU"
cstart="100"
cincr="100"
cend="2000"
rvals="1 3 7 15 31 63 127 255 511 1023 2047 4095 8191 16383"


for R in $rvals 
do
    for trc in $(ls lirs-trc/multi1.trc)
    do
        for alg in $algs
        do
            for c in $(seq $cstart $cincr $cend)
            do 
                echo  $trc $alg $c
                python2 cache.py -c $alg -n $c -t $trc -R $R
            done
        done
    done
done
