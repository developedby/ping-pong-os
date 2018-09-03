// Implementação das estruturas definidas em queue.h
// Nicolas Abril e Lucca Rawlyk

#include <stdio.h>
#include "datatypes.h"		// estruturas de dados necessárias
#include <ucontext.h>
#include "pingpong.h"

#define CONTEXT_STACKSIZE 32768


int next_tid;
task_t *task_queue;
task_t *current_task;
task_t main_task;

void pingpong_init ()
{
  /* desativa o buffer da saida padrao (stdout), usado pela função printf */
  setvbuf (stdout, 0, _IONBF, 0) ;

  // Cria a main task
  main_task.next = NULL;
  main_task.prev = NULL;
  queue_append((queue_t**) &task_queue, (queue_t*) main_task);
  next_tid = 0;
  main_task.tid = next_tid++;
  main_task.parent = NULL;

  current_task = &main_task;
  return;
}

int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
  task = malloc(sizeof(task_t));
  task->next = NULL;
  task->prev = NULL;
  queue_append((queue_t**) &task_queue, (queue_t*) task);

  task->tid = next_tid++;

  task->parent = &main_task;

  getcontext(task->context);
  makecontext(task->context, void (*start_func), 1, arg);
  char *stack = malloc (STACKSIZE) ;
  if (stack)
  {
     ContextPong.uc_stack.ss_sp = stack ;
     ContextPong.uc_stack.ss_size = CONTEXT_STACKSIZE;
     ContextPong.uc_stack.ss_flags = 0;
     ContextPong.uc_link = 0;
  }
  else
  {
     perror ("Erro na criacao da pilha: ");
     exit (1);
  }

  return tid;
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
