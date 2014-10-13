#ifndef STATISTICS_H
#define STATISTICS_H



#define USE_GHOSTLIST 1
//#define USE_GHOSTLIST 0
#define USE_ROUNDER 1
// #define USE_GLOBAL_FILTER
#define USE_GLOBAL_PLUS 1
#define DO_SANITY 0


#ifdef FORCE_NO_GHOSTLIST // memcached-mimir-noghost
#undef USE_GHOSTLIST
#endif

/* Use background thread? */
//#define MIMIR_BACKGROUND_THREAD


#define MIMIR_TYPE_EVICT	1
#define MIMIR_TYPE_MISS		2

// two things: if we had 2 value cas we wouldn't need it (DCAS)
// wrapping the value and the mark into one struct to be able to use a single CAS instruction


// each bucket contains a mark and a counter
// the mark denotes whether some thread is doing aging
// the counter is the number of entries in the bucket

#define GET_mark(cm) ((cm) & 0x1)
#define GET_count(cm) ((cm) >> 1)
#define GET_count_and_mark(count, mark) (((count) << 1 ) | ((mark) & 0x1)) // just to make sure that mark is 0 or 1
#define SET_count_and_mark(x, count, mark) ( x = GET_count_and_mark(count, mark))



#endif
