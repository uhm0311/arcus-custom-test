CC = g++
CFLAGS = -w -I/home/uhmin/arcus/include -I/Users/jam2in/Repositories/arcus/include
LFALGS = -L/home/uhmin/arcus/lib -L/Users/jam2in/Repositories/arcus/lib -lhashkit -lmemcached -lmemcachedprotocol -lmemcachedutil -lzookeeper_mt -lpthread
CCC = $(CC) $(CFLAGS) $(LFALGS) $^ -o $@
OUTPUT = connection_test multi_process_test multi_threaded_test test speed_test

all: $(OUTPUT)

connection_test: connection_test.c
	$(CCC)

multi_process_test: multi_process_test.c
	$(CCC)

multi_threaded_test: multi_threaded_test.c
	$(CCC)

speed_test: speed_test.c
	$(CCC)

test: test.cc
	$(CCC)

clean:
	rm -f $(OUTPUT)
