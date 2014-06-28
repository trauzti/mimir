#!/bin/sh
set -e

function waitfunc {
    set +e
    pid=$1
    echo "waiting for $pid"
    while true 
    do
        [[ $(ps | grep $pid) ]] || break
        sleep 0.1s
    done
    set -e
    echo "done waiting"
}

C=100000
bits="3 4 5 6 7"
#bits="3 4 5 6 7 8 9 10"
#bits="8 9 10"
olddir="$(pwd)"

for b in $bits
do
    echo "bits=$b"
    ./memcached -m 20 -T $C -q $b -r 2&>1 1> "output/memcached__output__bits=${b}.txt"  &
    memcachedpid="$!"
    echo "Started memcached with pid=($memcachedpid)"
    cd /home/trauzti/UnSynced/ycsb-memcached
    echo "Starting java"
    (java -cp build/ycsb.jar:db/memcached/lib/spymemcached-2.7.3.jar com.yahoo.ycsb.Client -t -P workloads/mc.b2) > "${olddir}/output/ycsb__output__bits=${b}.txt" &
    javapid="$!"
    echo "Waiting for YCSB to finish pid(${javapid})"
    waitfunc "$javapid"
    echo "sending stats coolpdf to memcached"
    echo "stats coolpdf" | nc localhost 11211
    echo "Killing memcached"
    kill -9 $memcachedpid
    cd -
    ../simplegraph.py -a naive.txt -b rounder.txt -A naive -B "rounder(bits=${b})" -p "output/graph__bits=${b}" > "output/accuracy__bits=${b}.txt"
    mv naive.txt "output/naive__bits=${b}.txt"
    mv rounder.txt "output/rounder__bits=${b}.txt"
done
