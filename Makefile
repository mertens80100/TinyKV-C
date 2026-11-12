CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2 -Iinclude

all: tinykv

tinykv: src/main.c src/kv_store.c include/kv_store.h
	$(CC) $(CFLAGS) src/main.c src/kv_store.c -o tinykv

tinykv_test: tests/test_kv.c src/kv_store.c include/kv_store.h
	$(CC) $(CFLAGS) tests/test_kv.c src/kv_store.c -o tinykv_test

test: tinykv_test
	./tinykv_test

clean:
	rm -f tinykv tinykv_test *.wal

.PHONY: all test clean
