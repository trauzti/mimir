#!/bin/sh

TRACE=P10.lis
#TRACE=P11.lis
#TRACE=long_w2.asc.rdtr

mkdir -p output

runs=10
#runs=40
for r in $(seq $runs)
do
    fn="lru_regular_${r}"
    printf "$fn   \r"
    #./syslab_cache -c LRU_LINKEDLIST -s BASIC -n 5000 -f $TRACE >output/$fn 2>&1
    fn="lru_rounder_${r}"
    printf "$fn   \r"
    ./syslab_cache -c LRU_LINKEDLIST -s ROUNDER -n 5000 -f $TRACE >output/$fn 2>&1
    fn="lru_avl_${r}"
    printf "$fn   \r"
    #./syslab_cache -c LRU_AVL -s BASIC -n 5000 -f $TRACE >output/$fn 2>&1
    fn="lru_mattson_${r}"
    printf "$fn   \r"
    #./syslab_cache -c LRU_MATTSON -s BASIC -n 5000 -f $TRACE >output/$fn 2>&1
done
printf "\033[J"


#grep Throughput output/* |sed 's/_/ /g' | gawk '{sum[$2] += $4;cnt[$2]++} END {for (i in sum) { print i "\t" sum[i]/cnt[i] "\t" 100*sum[i] / sum["regular"] "%" } }'

