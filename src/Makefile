CC = gcc
CFLAGS = -Wall
OBJS = commnetlink.o

all: commnetlink

%.o: %.c
	$(CC) $(CFLAGS) -c -g -o $@ $<

commnetlink: $(OBJS)
	$(CC) -o $@ $(OBJS)

clean:
	rm -f commnetlink *.o
