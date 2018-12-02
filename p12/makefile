CC=gcc
CFLAGS=-Wall

pingpong-os: queue.c pingpong.c pingpong-mqueue.c
	$(CC) -o pingpong-os queue.c pingpong.c pingpong-mqueue.c

debug: queue.c pingpong.c pingpong-mqueue.c
	$(CC) -g -o pingpong-os queue.c pingpong.c pingpong-mqueue.c
