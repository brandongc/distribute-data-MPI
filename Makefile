TARGETS=distribute-data memory-estimate
OBJS=distribute-data.o bins.o

CC=cc
CFLAGS=-Wall

.PHONY: all clean

all: $(TARGETS)

distribute-data: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

memory-estimate: memory-estimate.c
	gcc -o $@ $^

clean:
	rm -f $(TARGETS) $(OBJS)
