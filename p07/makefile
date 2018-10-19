CC=gcc
CFLAGS=-Wall

pingpong-os: queue.c pingpong.c pingpong-maintask.c
	$(CC) -o pingpong-os queue.c pingpong.c pingpong-maintask.c

debug: queue.c pingpong.c pingpong-maintask.c
	$(CC) -g -o pingpong-os queue.c pingpong.c pingpong-maintask.c
