#!/bin/sh

mkdir -p output

runs=10
for r in $(seq $runs)
do
    fn="lru_regular_${r}"
    ./syslab_cache -c LRU_LINKEDLIST -s BASIC -n 5000 -f /huge/Research/cache/first/P10.lis >output/$fn 2>&1
    fn="lru_rounder_${r}"
    ./syslab_cache -c LRU_LINKEDLIST -s ROUNDER -n 5000 -f /huge/Research/cache/first/P10.lis >output/$fn 2>&1
    fn="lru_avl_${r}"
    ./syslab_cache -c LRU_AVL -s BASIC -n 5000 -f /huge/Research/cache/first/P10.lis >output/$fn 2>&1
    fn="lru_mattson_${r}"
    ./syslab_cache -c LRU_MATTSON -s BASIC -n 5000 -f /huge/Research/cache/first/P10.lis >output/$fn 2>&1
done
