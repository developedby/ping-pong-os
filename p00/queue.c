// Implementação das estruturas definidas em queue.h
// Nicolas Abril e Lucca Rawlyk

#include "queue.h"
#include <stdio.h>

void queue_append (queue_t **queue, queue_t *elem)
{
  // Verifica se as entradas sao validas
    if (!queue)
    {
      printf("queue_append: Erro - queue não existe.\nqueue: %p\telem: %p", queue, elem);
      return;
    }
    if (!elem)
    {
      printf("queue_append: Erro - elem não existe.\nqueue: %p\telem: %p", queue, elem);
      return;
    }
    if (elem->next || elem->prev)
    {
      printf("queue_append: Erro - elem está em outra fila.\nqueue: %p\t elem: %p\nelem->next: %p\t elem->prev: %p", queue, elem, elem->next, elem->prev);
      return;
    }

  // Insere elemento na fila
    // Se a fila está vazia
      if (!*queue)
      {
        *queue = elem;
        (*queue)->next = (*queue);
        (*queue)->prev = (*queue);
        return;
      }

    // Se a fila não esta vazia
      queue_t *iter = (*queue);
      while(iter->next != (*queue)) // Vai ate o ultimo elemento da fila
      {
        iter = iter->next;
      }
      iter->next = elem; // Insere elem na fila
      elem->prev = iter;
      elem->next = (*queue);
      (*queue)->prev = elem;
}

queue_t *queue_remove (queue_t **queue, queue_t *elem)
{
  // Verifica se as entradas sao validas
    if (!queue)
    {
      printf("queue_remove: Erro - queue não existe.\nqueue: %p\telem: %p", queue, elem);
      return NULL;
    }
    if (!*queue)
    {
      printf("queue_remove: Erro - queue vazia.\nqueue: %p\telem: %p\t*queue: %p", queue, elem, *queue);
      return NULL;
    }
    if (!elem)
    {
      printf("queue_remove: Erro - elem não existe.\nqueue: %p\telem: %p", queue, elem);
      return NULL;
    }
    queue_t *iter = *queue;
    char found_elem = 0;
    do
    {
      if (iter == elem)
      {
        found_elem = 1;
        break;
      }
      iter = iter->next;
    }while (iter->next != *queue);
    if (found_elem == 0)
    {
      printf("queue_append: Erro - elem não está na fila.\nqueue: %p\t elem: %p\n", queue, elem);
      return NULL;
    }

    // Se elemento e o primeiro da fila, move o ponteiro
    if (elem == *queue)
    {
      *queue = elem->next;
      if((*queue)->next == *queue)
      {
        *queue = NULL;
      }
    }
    // Remove elemento da fila
      (elem->prev)->next = elem->next;
      (elem->next)->prev = elem->prev;
      elem->prev = NULL;
      elem->next = NULL;
      return elem;
}

int queue_size (queue_t *queue)
{
  if (queue)
  {
    int size = 1;
    queue_t *iter = queue;
    while(iter->next != queue)
    {
      iter = iter->next;
      size++;
    }
    return size;
  }
  return 0;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*) )
{
  printf("%s", name);
  if (queue)
  {
    queue_t *iter = queue;
    do
    {
      print_elem ((void *) iter);
      iter = iter->next;
    }while(iter != queue);
    return;
  }
}
