CFLAGS = -Wall -g -O3 -lm -lSDL2 -lSDL2_mixer -std=c99 -pedantic

ray-cast-1: ray-cast-1.c
	gcc ray-cast-1.c -o ray-cast-1 $(CFLAGS)

