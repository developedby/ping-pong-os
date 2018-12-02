CC=gcc
CFLAGS=-Wall

pingpong-os: queue.c pingpong.c pingpong-semaphore.c pingpong-racecond.c
	$(CC) -o pingpong-os1 queue.c pingpong.c pingpong-semaphore.c
	$(CC) -o pingpong-os2 queue.c pingpong.c pingpong-racecond.c

debug: queue.c pingpong.c pingpong-semaphore.c pingpong-racecond.c
	$(CC) -g -o pingpong-os1 queue.c pingpong.c pingpong-semaphore.c
	$(CC) -g -o pingpong-os2 queue.c pingpong.c pingpong-racecond.c
