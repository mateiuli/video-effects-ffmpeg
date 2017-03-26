CC=gcc
CFLAGS=-g -std=gnu99
FFMPEG=-lavformat -lavcodec -lavutil -lswscale -lm

build: main
 
main: main.c ctve.c blur.c effects.c
	$(CC) $(CFLAGS) $^ -o $@ $(FFMPEG)

encode_example	: encode_example.c
	$(CC) $(CFLAGS) $^ -o $@ $(FFMPEG)

crap: crap.c
	$(CC) $(CFLAGS) $^ -o $@ $(FFMPEG)	

clean:
	rm -rf main

