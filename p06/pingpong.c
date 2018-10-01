/* Implementação das estruturas definidas em queue.h */
/* Nicolas Abril e Lucca Rawlyk */

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>

#include "queue.h"

#include "pingpong.h"

#define CONTEXT_STACKSIZE 32768

#define QUANTUM_SIZE 20
#define PREEMPTION_TIMER_FIRST_TICK_USEC 1000
#define PREEMPTION_TIMER_INTERVAL_USEC 1000

void dispatcher_body ();
task_t* scheduler ();

int next_tid;
task_t *ready_queue;
task_t *suspended_queue;
task_t *current_task;
task_t dispatcher;

struct sigaction preemption_action;
struct itimerval preemption_timer;
int preemption_counter;
unsigned int system_time;

void preemption_tick ()
{
  system_time ++;
  if (current_task != NULL)
  {
    current_task->proc_time ++;
    if (current_task->owner_uid != 0)
    {
      if (preemption_counter > 1)
      {
        preemption_counter--;
      }
      else
      {
        task_switch(&dispatcher);
      }
    }
  }
  return;
}

void pingpong_init ()
{
  /* desativa o buffer da saida padrao (stdout), usado pela função printf */
  setvbuf (stdout, 0, _IONBF, 0) ;
  // Inicializa variaveis globais
  current_task = NULL;
  next_tid = 0;

  // Cria o dispatcher
  task_create(&dispatcher, dispatcher_body, 0);
  dispatcher.owner_uid = 0; // Marca como tarefa de sistema

  // Configura a preempcao por temporizador
  preemption_counter = QUANTUM_SIZE;
  preemption_action.sa_handler = preemption_tick ;
  sigemptyset (&preemption_action.sa_mask) ;
  preemption_action.sa_flags = 0 ;
  if (sigaction (SIGALRM, &preemption_action, 0) < 0)
  {
    perror ("Erro em sigaction: ") ;
    exit (1) ;
  }

  // ajusta valores do temporizador
  preemption_timer.it_value.tv_usec = PREEMPTION_TIMER_FIRST_TICK_USEC ;      // primeiro disparo, em micro-segundos
  preemption_timer.it_value.tv_sec  = 0 ;      // primeiro disparo, em segundos
  preemption_timer.it_interval.tv_usec = PREEMPTION_TIMER_INTERVAL_USEC ;   // disparos subsequentes, em micro-segundos
  preemption_timer.it_interval.tv_sec  = 0 ;   // disparos subsequentes, em segundos

  system_time = 0;

  // arma o temporizador ITIMER_REAL (vide man setitimer)
  if (setitimer (ITIMER_REAL, &preemption_timer, 0) < 0)
  {
    perror ("Erro em setitimer: ") ;
    exit (1) ;
  }


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
  task->owner_uid = -1; // Teria que receber como parametro, mas nao posso alterar a definicao de task_create
                        // Marca como usuario por padrao, pelo motivo acima
  task->s_prio = 0;
  task->d_prio = task->s_prio;

  task->initial_systime = systime();
  task->proc_time = 0;
  task-> activations = 0;

  task->exec_state = READY;

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

  preemption_counter = QUANTUM_SIZE;

  queue_remove((queue_t**)&ready_queue, (queue_t*)current_task);
  current_task->queue = NULL;
  current_task->exec_state = RUNNING;
  current_task->activations++;

  if(old_task != NULL)
  {
    queue_append((queue_t**)&ready_queue, (queue_t*)old_task);
    old_task->queue = &ready_queue;
    old_task->exec_state = READY;
    old_task->d_prio = old_task->s_prio + 1; // d_prio e incrementado para consertar o envelhecimento
    return swapcontext(old_task->context, task->context);
  }
  else
  {
    ucontext_t trash;
    return swapcontext(&trash, task->context);
  }

}

void task_exit (int exitCode)
{
  printf( "Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",
          current_task->tid,
          systime() - current_task->initial_systime, 
          current_task->proc_time, 
          current_task->activations );

  free(current_task->context->uc_stack.ss_sp);
  // Se terminou de executar tudo, nao libera o dispatcher (alocacao estatica)
  // nem volta para ele, já que terminou
  if(current_task != &dispatcher)
  {
    //Nao pode dar free porque as tasks foram criadas estaticamente
    //free(current_task);
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
    current_task = &dispatcher;

    preemption_counter = QUANTUM_SIZE;

    old_task->exec_state = SUSPENDED;
    old_task->queue = queue;
    queue_append((queue_t**)queue, (queue_t*)old_task);

    queue_remove((queue_t**)dispatcher.queue, (queue_t*)&dispatcher);
    dispatcher.exec_state = RUNNING;
    dispatcher.queue = NULL;

    swapcontext(old_task->context, dispatcher.context);
  }
  else
  {
    task->exec_state = SUSPENDED;
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
  task->exec_state = READY;
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
  task_t *next_task;
  while (queue_size( (queue_t*)ready_queue ) > 0)
  {
   
    // Troca para a proxima tarefa
    next_task = scheduler();
    if (next_task != NULL)
    {

      task_switch(next_task); // transfere controle para a tarefa "next"
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
  task_t *highest_prio_task = ready_queue; // highest_prio e o menor numero
  task_t *iter;

  // Envelhece as tasks e seleciona a de maior prioridade
  // A primeira task a ser executada tem desvantagem porque as outras ja estao envelhecidas em 1 (ocorre uma vez no inicio do sistema)
  // Pode dar overflow se envelhecer muito
  ready_queue->d_prio -= 1;
  for (iter = ready_queue->next; iter != ready_queue; iter = iter->next)
  {
    iter->d_prio -= 1;
    if (iter->d_prio < highest_prio_task->d_prio) // Prioridade negativa
    {
      highest_prio_task = iter;
    }
  }
  return highest_prio_task;
}

void task_setprio (task_t *task, int new_s_prio)
{
  if (task == NULL)
  {
    if (current_task != NULL)
    {
    	task = current_task;
		}
		else
		{
			printf("Tentou alterar prioridade do vazio, ignorado\n");
			return;
		}
  }
  // Mantem o envelhecimento quando troca s_prio
  task->d_prio -= task->s_prio;

  task->s_prio = new_s_prio;
  if (task->s_prio > 20)
  {
    task->s_prio = 20;
  }
  else
  {
    if (task->s_prio < -20)
    {
      task->s_prio = -20;
    }
  }
  // Atualiza a prioridade dinamica para bater com a atualizacao
  task->d_prio += task->s_prio;
  return;
}

int task_getprio (task_t *task)
{
  if (task == NULL)
  {
		if (current_task != NULL)
    {
    	return current_task->s_prio;
		}
		else
		{
			printf("Tentou alterar prioridade do vazio, retornando NULL\n");
			return NULL;
		}
  }
  return task->s_prio;
}

unsigned int systime ()
{
  return system_time;
}
