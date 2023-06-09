// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional
// Modificado do original por Nicolas Abril e Lucca Rawlyk

#ifndef __DATATYPES__
#define __DATATYPES__

#include <ucontext.h>

enum task_state { READY=0, SUSPENDED, RUNNING };

// Estrutura que define uma tarefa
typedef struct task_t
{
  struct task_t *prev, *next;
  struct task_t **queue;
  int tid;
  int owner_uid; // id do dono da tarefa; 0 = tarefa de sistema  
  int s_prio; // Prioridade estatica
  int d_prio; // Prioridade dinamica
  //struct task_t *parent;
  enum task_state state;
  ucontext_t *context;
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif
