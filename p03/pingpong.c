/* Implementação das estruturas definidas em queue.h */
/* Nicolas Abril e Lucca Rawlyk */

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#include "queue.h"

#include "pingpong.h"

#define CONTEXT_STACKSIZE 32768

int next_tid;
task_t *ready_queue;
task_t *suspended_queue;
task_t *current_task;
task_t dispatcher;

void pingpong_init ()
{
  /* desativa o buffer da saida padrao (stdout), usado pela função printf */
  setvbuf (stdout, 0, _IONBF, 0) ;

  current_task = NULL;
  next_tid = 0;

  task_create(&dispatcher, dispatcher_body, 0)

  return;
}

int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
  // task ja tem um espaco de memoria alocado para ela quando entra nessa funcao
  //task = malloc(sizeof(task_t));
  task->next = NULL;
  task->prev = NULL;
  queue_append((queue_t**) &ready_queue, (queue_t*) task);
  task->queue = &ready_queue;

  task->tid = next_tid++;

  task->state = READY;

  task->context = malloc(sizeof(ucontext_t));
  getcontext(task->context);
  char *stack = malloc(CONTEXT_STACKSIZE);
  if (stack)
  {
     task->context->uc_stack.ss_sp = stack ;
     task->context->uc_stack.ss_size = CONTEXT_STACKSIZE;
     task->context->uc_stack.ss_flags = 0;
     task->context->uc_link = 0;
  }
  else
  {
     perror ("Erro na criacao da pilha: ");
     exit (1);
  }
  makecontext(task->context, (void*)start_func, 1, arg);

  return task->tid;
}

int task_switch (task_t *task)
{
  task_t* old_task = current_task;
  current_task = task;

  queue_remove((queue_t**)&ready_queue, (queue_t*)current_task);
  current_task->queue = NULL;
  current_task->state = RUNNING;

  if(old_task != NULL)
  {
    queue_append((queue_t**)&ready_queue, (queue_t*)old_task);
    old_task->queue = &ready_queue;
    old_task->state = READY;
  }

  return swapcontext(old_task->context, task->context);
}

void task_exit (int exitCode)
{
  free(current_task->context->uc_stack.ss_sp);
  // Se terminou de executar tudo, nao libera o dispatcher (alocacao estatica)
  // nem volta para ele, porque terminou
  if(current_task != &dispatcher)
  {
    free(current_task);
    current_task = NULL;
    task_switch(&dispatcher);
  }
  return;
}

int task_id ()
{
  return current_task->tid;
}

void task_suspend (task_t *task, task_t **queue)
{
  // Da erro se tenta suspender o dispatcher
  if(task == &dispatcher || ((task == NULL) && (current_task == &dispatcher)))
  {
    printf("task_suspend: Tentou suspender o dispatcher, ignorado\n");
    return;
  }

  if (task == NULL || task == current_task)
  {
    task_t *old_task = current_task;
    current_task = dispatcher;

    old_task->state = SUSPENDED;
    old_task->queue = queue;
    queue_append((queue_t**)queue, (queue_t*)old_task);

    queue_remove((queue_t**)dispatcher.queue, (queue_t*)&dispatcher);
    dispatcher.state = RUNNING;
    dispatcher.queue = NULL;

    swapcontext(old_task->context, dispatcher.context);
  }
  else
  {
    task->state = SUSPENDED;
    task->queue = queue;
    queue_remove((queue_t**)task->queue, (queue_t*)task);
    queue_append((queue_t**)queue, (queue_t*)task);
  }
  return;
}

void task_resume (task_t *task)
{
  // Nao pode dar task_resume na task que esta rodando
  if (task == current_task)
  {
    printf("task_resume: Tentou retomar a task que ja esta rodando, ignorado\n");
    return;
  }
  // Se a tarefa ja esta pronta, nao precisa fazer nada
  if (task->queue == &ready_queue)
  {
    return;
  }

  queue_remove((queue_t**)task->queue, (queue_t*)task);
  queue_append((queue_t**)&ready_queue, (queue_t*)task);
  task->state = READY;
  task->queue = &ready_queue;
  return;
}

void task_yield ()
{
  task_switch(&dispatcher);
  return;
}

void dispatcher_body ()
{
  while (queue_size( (queue_t**)&ready_queue ) > 0)
  {
    next = scheduler();
    if (next != NULL)
    {
      task_switch(next); // transfere controle para a tarefa "next"
    }
    else
    {
      printf("dispatcher_body: Próxima tarefa não existe\n");
    }
  }
  task_exit(0); // encerra a tarefa dispatcher
  return;
}

task_t* scheduler ()
{
  return ready_queue;
}
