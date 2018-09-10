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
task_t *current_task;
task_t dispatcher;

void pingpong_init ()
{
  /* desativa o buffer da saida padrao (stdout), usado pela função printf */
  setvbuf (stdout, 0, _IONBF, 0) ;

  current_task = &dispatcher;
  next_tid = 0;

  /* Cria o dispatcher */
  dispatcher.next = NULL;
  dispatcher.prev = NULL;
  dispatcher.tid = next_tid++;
  dispatcher.parent = NULL;
  dispatcher.context = malloc(sizeof(ucontext_t));
  getcontext(dispatcher->context);
  char *stack = malloc (CONTEXT_STACKSIZE) ;
  if (stack)
  {
     dispatcher->context->uc_stack.ss_sp = stack ;
     dispatcher->context->uc_stack.ss_size = CONTEXT_STACKSIZE;
     dispatcher->context->uc_stack.ss_flags = 0;
     dispatcher->context->uc_link = 0;
  }
  else
  {
     perror ("Erro na criacao da pilha: ");
     exit (1);
  }
  makecontext(dispatcher->context, (void*)dispatcher_body, 0);

  return;
}

int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
  // task ja tem um espaco de memoria alocado para ela quando entra nessa funcao
  //task = malloc(sizeof(task_t));
  task->next = NULL;
  task->prev = NULL;
  queue_append((queue_t**) &ready_queue, (queue_t*) task);

  task->tid = next_tid++;

  task->parent = &dispatcher;

  task->context = malloc(sizeof(ucontext_t));
  getcontext(task->context);
  char *stack = malloc (CONTEXT_STACKSIZE) ;
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
  return swapcontext(old_task->context, task->context);
}

void task_exit (int exitCode)
{
  queue_remove((queue_t**) &task_queue, (queue_t*) current_task);
  task_switch(current_task->parent);
  return;
}

int task_id ()
{
  return current_task->tid;
}
