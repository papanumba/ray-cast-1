CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -O3
LDLIBS = -lm -lSDL2 -lSDL2_mixer

ray-cast-1: main.c
	 $(CC) $(CFLAGS) main.c -o ray-cast-1 $(LDLIBS)
