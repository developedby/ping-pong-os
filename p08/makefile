CC=gcc
CFLAGS=-Wall

pingpong-os: queue.c pingpong.c pingpong-join.c
	$(CC) -o pingpong-os queue.c pingpong.c pingpong-join.c

debug: queue.c pingpong.c pingpong-join.c
	$(CC) -g -o pingpong-os queue.c pingpong.c pingpong-join.c
