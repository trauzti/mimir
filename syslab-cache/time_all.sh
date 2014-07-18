#!/bin/sh

#cd /home/trausti12/mimir/syslab-cache

#TRACE=P10.lis
TRACE=/dev/shm/long_w2.asc.rdtr
#TRACE=P11.lis
#TRACE=long_w2.asc.rdtr

#rvals="1 17 121 251 1023 2040 10001 100001"
#rvals="1 3 7 15 31 63 127 255 511 1023 2047 4095 8191 16383 32767 65535 131071 262143 524287 1048575 2097151 4194303 8388607 16777215 33554431 67108863 134217727 268435455 536870911 1073741823 2147483647 4294967295"
#rvals="1"
#rvals="1 17 121 251 1023 2040 10001 100001"
rvals="1 3 7 15 31 63 127 255 511 1023 2047 4095 8191 16383 32767 65535 131071" #262143 524287 1048575 2097151 4194303 8388607 16777215 33554431 67108863 134217727 268435455 536870911 1073741823 2147483647 4294967295"

#runs=10
runs=40
for R in $rvals
do
    echo "Doing R=${R}"
    mkdir -p output
    for run in $(seq $runs)
    do
    mkdir -p output
        fn="lru_regular_${run}"
        printf "$fn   \r"
        ./syslab_cache -c LRU_LINKEDLIST -s BASIC -n 5000 -R $R -f $TRACE >output/$fn 2>&1
        fn="lru_rounder_${run}"
        printf "$fn   \r"
        ./syslab_cache -c LRU_LINKEDLIST -s ROUNDER -n 5000 -R $R -f $TRACE >output/$fn 2>&1
        fn="lru_avl_${run}"
        printf "$fn   \r"
        ./syslab_cache -c LRU_AVL -s BASIC -n 5000 -R $R -f $TRACE >output/$fn 2>&1
        fn="lru_mattson_${run}"
        printf "$fn   \r"
        ./syslab_cache -c LRU_MATTSON -s BASIC  -n 5000 -R $R -f $TRACE >output/$fn 2>&1
    done
    python2 parse_output.py > overhead.txt
    cp -r output output_R_${R}
    mv overhead.* output_R_${R}
done
printf "\033[J"


#grep Throughput output/* |sed 's/_/ /g' | gawk '{sum[$2] += $4;cnt[$2]++} END {for (i in sum) { print i "\t" sum[i]/cnt[i] "\t" 100*sum[i] / sum["regular"] "%" } }'

