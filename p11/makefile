CC=gcc
CFLAGS=-Wall

pingpong-os: queue.c pingpong.c pingpong-barrier.c
	$(CC) -o pingpong-os queue.c pingpong.c pingpong-barrier.c

debug: queue.c pingpong.c pingpong-barrier.c
	$(CC) -g -o pingpong-os queue.c pingpong.c pingpong-barrier.c
