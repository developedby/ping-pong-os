// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional
// Modificado do original por Nicolas Abril e Lucca Rawlyk

#ifndef __DATATYPES__
#define __DATATYPES__

#include <ucontext.h>

enum task_execution_state { READY, SUSPENDED, RUNNING, FINISHED };
enum obj_state { CREATED=548793, DESTROYED };

// Estrutura que define uma tarefa
typedef struct task_t
{
  struct task_t *prev, *next;       // Elementos adjacentes na fila desta tarefa
  struct task_t **queue;            // Fila de execução em que a tarefa pertence
  int tid;                          // Task id
  int owner_uid;                    // id do dono da tarefa; 0 = tarefa de sistema
  int s_prio;                       // Prioridade estatica
  int d_prio;                       // Prioridade dinamica
  //struct task_t *parent;
  unsigned int initial_systime;     // System_time na criação da tarefa
  unsigned int proc_time;           // Tempo que a tarefa passou efetivamente rodando
  unsigned int activations;         // Numero de vezes que a tarefa foi a tarefa atual
  unsigned int wake_time;           // Tempo que a tarefa vai ter que acordar quando for dormir
  enum task_execution_state exec_state; // Estado de execução da tarefa (tem que bater com a fila)
  struct task_t *waiting_queue;     // Fila de outros processos esperando por este processo
  int exit_code;                    // Código de saída gerado em task_exit
  ucontext_t *context;
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  int count;
  int max_count;
  task_t *queue;
  enum obj_state state;
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  int max_count;
  int count;
  task_t *queue;
  enum obj_state state;
} barrier_t ;

// estrutura que define uma fila de mensagens
//struct mqueue_elem_t;
typedef struct mqueue_t
{
  semaphore_t send_sem;
  semaphore_t recv_sem;
  int msg_size;
  int msg_count;
  struct mqueue_elem_t *queue;
  enum obj_state state;
} mqueue_t ;

typedef struct mqueue_elem_t
{
  struct mqueue_elem_t *prev, *next;
  mqueue_t *queue;
  void *msg;
} mqueue_elem_t ;


#endif