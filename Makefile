TARGETS= memory-estimate distribute-data

MPICC=cc
CC=gcc
CFLAGS=-Wall

.PHONY: all clean

all: $(TARGETS)

distribute-data: distribute-data.c
	$(MPICC) $(CFLAGS) $(CPPFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)
