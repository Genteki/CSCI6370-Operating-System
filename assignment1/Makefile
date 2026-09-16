SHELL := /bin/bash

all: wc_mul

wc_mul: wc_mul.c
	$(CC) wc_mul.c -g -o wc_mul

test: wc_mul
	time ./wc_mul 1 large.txt
	time ./wc_mul 10 large.txt
	./wc_mul 10 large.txt 50

clean:
	rm -f wc_mul

.PHONY: all test bench clean
