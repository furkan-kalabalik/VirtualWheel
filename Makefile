CC=gcc
CFLAGS=-I.

controller: wheel.c main.c
	$(CC) -o controller wheel.c main.c