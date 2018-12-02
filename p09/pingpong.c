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
void task_queue_append(task_t *task, task_t **queue);
task_t* task_queue_remove(task_t *task, task_t **queue);
void task_queue_sort_last_element_by_wake_time(task_t **queue);

int next_tid;
task_t *ready_queue = NULL;
task_t *sleeping_queue = NULL;
task_t *current_task = NULL;
task_t dispatcher;
task_t main_task;

int initialized = 0;  // 1 se o sistema operacional já foi inicializado

struct sigaction preemption_action;
struct itimerval preemption_timer;
int preemption_counter;
int execution_lock = 0;

unsigned int system_time = 0;  //tempo que o sistema esta rodando em ms
int active_tasks = 0;

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
        // Se nao pode trocar de task
        if (execution_lock)
        {
          // Reseta o contador para nao interromper na hora que destravar as interrupcoes
          preemption_counter = QUANTUM_SIZE;

          return;
        }
        else
        {
          task_switch(&dispatcher);
        }
      }
    }
  }
}

void pingpong_init ()
{
  initialized = 0;
  system_time = 0;
  active_tasks = 0;

  // Impede que init seja interrompido
  execution_lock++;

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
  preemption_timer.it_value.tv_usec = PREEMPTION_TIMER_FIRST_TICK_USEC ;  // primeiro disparo, em micro-segundos
  preemption_timer.it_value.tv_sec  = 0 ;                                 // primeiro disparo, em segundos
  preemption_timer.it_interval.tv_usec = PREEMPTION_TIMER_INTERVAL_USEC ; // disparos subsequentes, em micro-segundos
  preemption_timer.it_interval.tv_sec  = 0 ;                              // disparos subsequentes, em segundos

  // arma o temporizador ITIMER_REAL (vide man setitimer)
  if (setitimer (ITIMER_REAL, &preemption_timer, 0) < 0)
  {
    perror ("Erro em setitimer: ") ;
    exit (1) ;
  }

  /* desativa o buffer da saida padrao (stdout), usado pela função printf */
  setvbuf (stdout, 0, _IONBF, 0) ;

  // Cria as tarefas iniciais
  next_tid = 0;
  task_create(&main_task, NULL, 0);
  //main_task.owner_uid = 0; // Marca a main como tarefa de sistema
  task_create(&dispatcher, dispatcher_body, 0);
  dispatcher.owner_uid = 0; // Marca dispatcher como tarefa de sistema

  // Inicia o sistema
  current_task = NULL;
  getcontext(main_task.context);
  if(initialized == 0)
  {
    initialized = 1;
    task_switch(&main_task); // Variavel global pra nao entrar em loop (como melhorar?)
  }
  execution_lock--;
  return;
}

int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
  execution_lock++;
  // task ja tem um espaco de memoria alocado para ela quando entra nessa funcao
  //task = malloc(sizeof(task_t));
  task->next = NULL;
  task->prev = NULL;
  task_queue_append(task, &ready_queue);

  task->tid = next_tid++;
  task->owner_uid = -1; // Teria que receber como parametro, mas nao posso alterar a definicao de task_create
                        // Marca como usuario por padrao, pelo motivo acima
  printf("Criando task %d no endereço %p\n", task->tid, task);

  task->s_prio = 0;
  task->d_prio = task->s_prio;

  task->initial_systime = systime();
  task->wake_time = 0;
  task->proc_time = 0;
  task->activations = 0;

  task->exec_state = READY;
  task->waiting_queue = NULL;
  task->exit_code = -1;

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

  active_tasks ++;

  execution_lock--;

  // Interrompe a tarefa atual para evitar inversao de prioridade
  // So deve ocorrer quando tiver escalonamento por prioridade
  // Tambem pode ser ruim com envelhecimento
  // Da erro *** stack smashing detected *** porque da yield na main, antes de existir o dispatcher
  //task_yield();

  return task->tid;
}

int task_switch (task_t *task)
{
  execution_lock++;
  task_t* old_task = current_task;
  current_task = task;

  preemption_counter = QUANTUM_SIZE;

  task_queue_remove(current_task, current_task->queue);
  current_task->exec_state = RUNNING;
  current_task->activations++;

  // Se nao tinha nada rodando, joga fora o contexto atual
  if(old_task == NULL)
  {
    ucontext_t trash;
    execution_lock--;
    return swapcontext(&trash, task->context);
  }
  // Se tinha, coloca a task antiga na fila e troca para a nova
  else
  {
    task_queue_append(old_task, &ready_queue);
    old_task->exec_state = READY;
    old_task->d_prio = old_task->s_prio + 1; // d_prio e incrementado para consertar o envelhecimento
    execution_lock--;
    return swapcontext(old_task->context, task->context);
  }


}

void task_exit (int exitCode)
{
  task_t *waiting_elem = NULL;
  if(current_task == NULL)
  {
    printf("task_exit - Nao tem nenhuma task rodando\n");
    exit(1);
  }

  execution_lock++;

  current_task->exit_code = exitCode;
  current_task->exec_state = FINISHED;
  active_tasks--;

  printf( "Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",
          current_task->tid,
          systime() - current_task->initial_systime,
          current_task->proc_time,
          current_task->activations );
  while (current_task->waiting_queue != NULL)
  {
    waiting_elem = current_task->waiting_queue;
    task_queue_remove(waiting_elem, &(current_task->waiting_queue));
    task_queue_append(waiting_elem, &ready_queue);
  }

  free(current_task->context->uc_stack.ss_sp);

  if(current_task != &dispatcher)
  {
    //Nao pode dar free porque as tasks foram criadas estaticamente
    //free(current_task);
    current_task = NULL;
    execution_lock--;
    task_switch(&dispatcher);
    return;
  }
  // Se terminou de executar tudo, nao libera o dispatcher (alocacao estatica)
  // Também nao volta para ele, já que terminou
  else
  {
    execution_lock--;
    return;
  }
}

int task_id ()
{
  return current_task->tid;
}

void task_suspend (task_t *task, task_t **queue)
{
  if (task == NULL)
  {
    task = current_task;
  }

  // Da erro se tenta suspender o dispatcher
  if(task == &dispatcher)
  {
    printf("task_suspend: Tentou suspender o dispatcher, ignorado\n");
    return;
  }

  // Se vai suspender a task atual, troca para o dispatcher
  if (task == current_task)
  {
    execution_lock++;
    task_t *old_task = current_task;
    current_task = &dispatcher;

    preemption_counter = QUANTUM_SIZE;

    old_task->exec_state = SUSPENDED;
    task_queue_append(old_task, queue);
    if (queue == &sleeping_queue)
    {
      task_queue_sort_last_element_by_wake_time(queue);
    }

    task_queue_remove(&dispatcher, dispatcher.queue);
    dispatcher.exec_state = RUNNING;
    dispatcher.activations++;

    execution_lock--;
    swapcontext(old_task->context, dispatcher.context);
  }
  // Se vai suspender uma task que não está executando só troca de fila
  else
  {
    execution_lock++;
    task->exec_state = SUSPENDED;
    task_queue_remove(task, task->queue);
    task_queue_append(task, queue);
    execution_lock--;
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
  execution_lock++;
  task_queue_remove(task, task->queue);
  task_queue_append(task, &ready_queue);
  task->exec_state = READY;
  execution_lock--;
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
  while (active_tasks > 1) // Deve ter um jeito melhor, 1 porque o dispatcher sempre existe
  {
    // Checa se tem alguem para acordar
    while (sleeping_queue && (sleeping_queue->wake_time < systime()))
    {
      task_resume(sleeping_queue);
    }  

    if (ready_queue)
    {
      // Troca para a proxima tarefa
      next_task = scheduler();
      if (next_task)
      {
        task_switch(next_task); // transfere controle para a tarefa "next"
      }
      // Proxima tarefa pode nao existir se nao tiver ninguem na fila de prontos
      /*else
      {
        printf("dispatcher_body: Warning - Próxima tarefa não existe\n");
      }*/ 
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
  execution_lock++;
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
  execution_lock--;
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
			printf("Tentou ler prioridade do vazio, retornando 0\n");
			return 0;
		}
  }
  return task->s_prio;
}

unsigned int systime ()
{
  return system_time;
}

int task_join (task_t *task)
{
  if (task == NULL)
  {
    return -1;
  }
  if (current_task == NULL)
  {
    printf("task_join - Warning: current task == NULL, ignorado\n");
    return -1;
  }
  if (task == current_task)
  {
    printf("task_join - Warning: Tentou esperar a propria task atual, ignorado\n");
    return -1;
  }
  if (task->exec_state == FINISHED)
  {
    return task->exit_code;
  }
  // Se a task está perdida, não espera
  if (task->queue == NULL)
  {
    printf("task_join - Warning: task nao esta em nenhuma fila, fingindo que terminou\n");
    return task->exit_code;
  }

  task_suspend(current_task, &(task->waiting_queue));

  return task->exit_code;
}

void task_sleep (int t)
{
  current_task->wake_time = systime() + t*1000;
  task_suspend(current_task, &sleeping_queue);
  return;
}



void task_queue_append(task_t *task, task_t **queue)
{
  execution_lock++;
  queue_append((queue_t**)queue, (queue_t*)task);
  task->queue = queue;
  execution_lock--;
}

task_t* task_queue_remove(task_t *task, task_t **queue)
{
  execution_lock++;
  task = (task_t*)queue_remove((queue_t**)queue, (queue_t*)task);
  if (task != NULL)
  {
    task->queue = NULL;
  }
  execution_lock--;
  return task;
}

void task_queue_sort_last_element_by_wake_time(task_t **queue)
{
  if (queue == NULL)
  {
    printf("task_queue_sort_last_element - Error: Fila não existe, ignorado\n");
    return;
  }
  if (*queue == NULL)
  {
    printf("task_queue_sort_last_element - Error: Tentou ordenar fila vazia, ignorado\n");
    return;
  }
  execution_lock++;
  task_t *last_elem = (*queue)->prev;
  for (task_t *elem = *queue; elem != last_elem; elem = elem->next)
  {
    if (elem->wake_time > last_elem->wake_time)
    {
      if (*queue == elem)
      {
        queue = &last_elem;
      }
      else
      {
        last_elem->prev->next = last_elem->next;
        last_elem->next->prev = last_elem->prev;
        last_elem->next = elem;
        last_elem->prev = elem->prev;
        elem->prev->next = last_elem;
        elem->prev = last_elem;
      }
      break;
    }
  }
  execution_lock--;
  return;
}