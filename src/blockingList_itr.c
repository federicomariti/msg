/**
 *  \file blockingList_itr.c
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */

#include <stdlib.h>
#include <errno.h>

#include "blockingList_itr.h"
#include "blockingList_private.h"


/** alloca ed inizializza un itreatore in memoria heap
 *
 */
blockingList_itr_t *
blockingList_itr_new(blockingList_t * list) /* TODO */
{
  return NULL;
}

/** inizializza un iteratore gia' allocato 
 *
 */
int
blockingList_itr_init(blockingList_itr_t * itr, blockingList_t * list)
{
  if (NULL == list || NULL == itr) {
    errno = EINVAL;
    return -1;
  }

  itr->list = list;

  itr->elem = NULL;
  
  itr->index = 0;

  if (0 == blockingList_getSize(list)) 
    return 0;

  itr->elem = list->head;


  return 0;
}

/** libera tutte le risorse occupate e la memoria heap occupata
 *  dall'iteratore
 *
 */
void
blockingList_itr_free(blockingList_itr_t ** itr) /* TODO */
{
  return ;
}

/** libera tutte le risorse occupate
 *
 */
int
blockingList_itr_destroy(blockingList_itr_t * itr)
{
  return 0;
}

/** ritorana il valore alla posizione corrente dell'iteratore della
 *  lista
 *
 */
void * 
blockingList_itr_getValue(blockingList_itr_t * itr)
{
  if (NULL == itr || NULL == itr->elem) {
    errno = EINVAL;
    return NULL;
  }

  return itr->elem->value;
}

/** itera la lista al prossimo elemento
 * 
 *  \param itr  riferimento all'iteratore
 *
 *  \retval 0   l'iterazione della lista e' terminata, l'iteratore e'
 *              sull'ultimo elemento
 *
 *  \retval -1  l'iterazione non e' terminata
 *
 */
int 
blockingList_itr_next(blockingList_itr_t * itr)
{
  if (NULL == itr ) {
    errno = EINVAL;
    return 0;
  }

  if (NULL == itr->elem)
    return 0;

  itr->elem = itr->elem->next;

  itr->index++;

  return -1;
}


/** ritorna vero (valore intero diverso da 0) se l'iteratore ha altri
 *  elementi
 *
 *  \retval !=0  esistono altri elementi nell'iteratore
 *
 *  \retval  0   altrimenti
 *
 */
int 
blockingList_itr_hasNext(blockingList_itr_t * itr)
{
  if (NULL == itr) {
    errno = EINVAL;
    return 0;
  }

  if (NULL == itr->elem)
    return 0;

  return NULL != itr->elem->next;
}

int 
blockingList_itr_remove(blockingList_itr_t * itr) /* TODO */
{
  return 0;
}

