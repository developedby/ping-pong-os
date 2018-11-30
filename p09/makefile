CC=gcc
CFLAGS=-Wall

pingpong-os: queue.c pingpong.c pingpong-sleep.c
	$(CC) -o pingpong-os queue.c pingpong.c pingpong-sleep.c

debug: queue.c pingpong.c pingpong-sleep.c
	$(CC) -g -o pingpong-os queue.c pingpong.c pingpong-sleep.c
