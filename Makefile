
CC = gcc
CFLAGS = -lhidapi

all:
	$(CC) $(CFLAGS) ds100.c -o ds100
