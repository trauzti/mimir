CC= g++-4.8
CFLAGS= -O3 -DSYSLAB_CACHE -DMIMIR -std=c++11 -g
LFLAGS= -levent -lpthread -ljemalloc -fpermissive -lrt
BINS= syslab_cache test_hashmap test_linkedlist test_avltree
OBJECTS= sbuf.o dablooms.o murmur3_hash.o hash.o statistics.o lru_linkedlist.o lru_avl.o lru_mattson.o common.o rounder.o readqueue.o #socket_server.o
INCLUDES= 

#all: clean
all: $(BINS)

debug: CFLAGS += -ggdb3 -pg
debug: CFLAGS += -Wall
debug: all

fast: CFLAGS += -DNDEBUG 
fast: all

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES)  -c -o $@ $<

%.o: %.cc
	$(CC) $(CFLAGS) $(INCLUDES)  -c -o $@ $<

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES)  -c -o $@ $<



syslab_cache: $(OBJECTS) 
	$(CC) $(CFLAGS) $(INCLUDES) syslab_cache.cpp $(OBJECTS) -o $@ $(LFLAGS)

test_hashmap: common.o hash.o
	$(CC) $(CFLAGS) $(INCLUDES) test_hashmap.cpp common.o hash.o -o $@ $(LFLAGS)

test_linkedlist: $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) test_linkedlist.cpp $(OBJECTS) -o $@ $(LFLAGS)

test_avltree: $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) test_avltree.cpp $(OBJECTS) -o $@ $(LFLAGS)


style:
	@astyle --style=java --indent=spaces=2 --pad-oper --pad-header --align-pointer=name --add-brackets --brackets=attach --convert-tabs *.cpp *.hpp


clean:
	rm -f *~ *.o  $(BINS) $(OBJECTS)
