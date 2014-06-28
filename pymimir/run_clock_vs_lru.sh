#!/bin/sh
algs="LRU CLOCK"
cstart="250"
cincr="250"
cend="3000"

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


