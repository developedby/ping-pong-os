/* Nicolas Abril e Lucca Rawlyk */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "pingpong.h"


semaphore_t s_buffer, s_item, s_vaga;
task_t prod[3], cons[2];
int my_buffer[5];
int buffer_head=0, buffer_tail=0, full_buffer = 0;

void prodBody ()
{
    int item;
    while (1)
    {
        task_sleep(1);
        item = random() % 100;
        sem_down(&s_vaga);
        sem_down(&s_buffer);

        my_buffer[buffer_tail] = item;
        buffer_tail = (buffer_tail+1) % 5;

        sem_up(&s_buffer);
        sem_up(&s_item);
        printf("prod t%d: %d\n", task_id(), item);
    }
}

void consBody ()
{
    int item;
    while (1)
    {
        sem_down(&s_item);
        sem_down(&s_buffer);

        item = my_buffer[buffer_head];
        buffer_head = (buffer_head+1) % 5;

        sem_up(&s_buffer);
        sem_up(&s_vaga);
        printf("\tcons t%d: %d\n", task_id(), item);
        task_sleep(1);
    }
}

int main ()
{
    printf ("Main INICIO\n") ;
    pingpong_init();

    sem_create(&s_buffer, 1);
    sem_create(&s_item, 0);
    sem_create(&s_vaga, 5);

    task_create(&cons[0], consBody, NULL) ;
    task_create(&cons[1], consBody, NULL) ;
    task_create(&prod[0], prodBody, NULL) ;
    task_create(&prod[1], prodBody, NULL) ;
    task_create(&prod[2], prodBody, NULL) ;

    task_join(&cons[1]);

    printf("Main FIM\n");
    task_exit(0);

    return 0;
}