/**
 *  \file blockingList.c
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "blockingList.h"
#include "blockingList_private.h"

void listElem_free(listElem_t ** elemp) {

  free((*elemp)->value);
  free(*elemp);

  *elemp = NULL;
}

int 
blockingList_init(blockingList_t * list, int maxSize, 
		  void * (* copy_fun)(void *),
		  int (* cmpr_fun)(void *, void *))
{
  if ( NULL == list || 0 > maxSize || NULL == copy_fun ||
       NULL == cmpr_fun ) {
    errno = EINVAL;
    return -1;
  }
  
  list->head = NULL;

  list->size = 0;

  list->maxSize = maxSize;

  list->copy_fun = copy_fun;

  list->cmpr_fun = cmpr_fun;

  pthread_mutex_init(&list->mtx, NULL);

  pthread_cond_init(&list->monitor, NULL);


  return 0;
}

blockingList_t *
blockingList_new(int maxSize, void * (* copy_fun)(void *),
		 int (* cmpr_fun)(void *, void *))
{
  blockingList_t * result = NULL;

  if (0 > maxSize || NULL == copy_fun || NULL == cmpr_fun) {
    errno = EINVAL;
    return NULL;
  }

  
  if (NULL == (result = malloc(sizeof(blockingList_t)))) 
    return NULL;

  if ( -1 == blockingList_init(result, maxSize, copy_fun, cmpr_fun))
    return NULL;


  return result;
}
  

int 
blockingList_destroy(blockingList_t * list)
{
  listElem_t * itr = NULL;

  if (NULL == list) {
    errno = EINVAL;
    return -1;
  }


  if ( -1 == pthread_mutex_lock(&list->mtx) )
    return -1;

 
  itr = list->head;

  while (itr) {
    listElem_t * toRmv = itr;
    itr = itr->next;
    _listElem_free(toRmv);
  }


  if ( -1 == pthread_mutex_unlock(&list->mtx) )
    return -1;


  pthread_mutex_destroy(&list->mtx); 
  pthread_cond_destroy(&list->monitor); /* sicuramente non ritorna errore */
  

  return 0;
}

void
blockingList_free(blockingList_t ** list)
{
  blockingList_destroy(*list);
  free(*list);
  *list = NULL;
}


int 
blockingList_lock(blockingList_t * list)
{
  if (NULL == list)
    return 0;

  return pthread_mutex_lock(&list->mtx);
}

int 
blockingList_unlock(blockingList_t * list)
{
  if (NULL == list)
    return 0;

  return pthread_mutex_unlock(&list->mtx);

}

int
blockingList_wait(blockingList_t * list)
{
  if (NULL == list)
    return 0;

  return pthread_cond_wait(&list->monitor, &list->mtx);
}

int 
blockingList_notify(blockingList_t * list)
{
  if (NULL == list) {
    errno = EINVAL;
    return -1;
  }
  
  return pthread_cond_signal(&list->monitor);
}

int 
blockingList_notifyAll(blockingList_t * list)
{
  if (NULL == list) {
    errno = EINVAL;
    return -1;
  }

  return pthread_cond_broadcast(&list->monitor);
}

int blockingList_isEmpty(blockingList_t * list) {
  if (NULL == list) {
    errno = EINVAL; 
    return -1;
  }

  return list->head == NULL;
}

int 
blockingList_push(blockingList_t * list, void * valuep)
{
  void * newElemValue = NULL;
  listElem_t * newElem = NULL;

  if (NULL == list || NULL == valuep) {
    errno = EINVAL;
    return -1; 
  }
  

  if ( -1 == pthread_mutex_lock(&list->mtx) )
    return -1;

  while (list->maxSize != 0  &&  
	 list->size == list->maxSize) 	/* list full */
  {
    errno = 0;
    pthread_cond_wait(&list->monitor, &list->mtx); 
    if (EINTR == errno) {
      pthread_mutex_unlock(&list->mtx);
      return -1;
    }
  }

  
  if ( NULL == (newElemValue = list->copy_fun(valuep)) ) {
    pthread_mutex_unlock(&list->mtx);
    return -1;
  }
  
  if ( NULL == (newElem = malloc(sizeof(listElem_t))) ) {
    free(newElemValue);
    pthread_mutex_unlock(&list->mtx);
    return -1;
  }

  _listElem_init(newElem, newElemValue, list->head);
  list->head = newElem;
  list->size++;

  pthread_cond_signal(&list->monitor); /* list isn't empty */

  if ( -1 == pthread_mutex_unlock(&list->mtx) )
    return -1;


  return 0;
}

int 
blockingList_pop(blockingList_t * list, void ** valuep)
{
  listElem_t * toRmv = NULL;

  if (NULL == list) {
    errno = EINVAL;
    return -1;
  }

  
  if ( -1 == pthread_mutex_lock(&list->mtx) )
    return -1;

  while (!list->head) 		/* list is empty */
  {
    errno = 0;
    pthread_cond_wait(&list->monitor, &list->mtx);
    if (EINTR == errno) {
      pthread_mutex_unlock(&list->mtx);
      return -1;
    }
    /* pthread_con_wait
     * If a signal is delivered to a thread waiting for a condition
     * variable, upon return from the signal handler the thread
     * resumes waiting for the condition variable as if it was not
     * interrupted, or it shall return zero due to spurious wakeup. 
     */
  }

  toRmv = list->head;
  list->head = toRmv->next;
  if ( NULL != valuep )
    *valuep = toRmv->value;
  else 
    free(toRmv->value);
  free(toRmv);
  list->size--;

  pthread_cond_signal(&list->monitor); /* list is't full */
  
  if ( -1 == pthread_mutex_unlock(&list->mtx) )
    return -1;

  
  return 0;
}

int
blockingList_poll(blockingList_t * list, void ** valuep)
{
  return blockingList_pop(list, valuep);
}

int 
blockingList_add(blockingList_t * list, void * valuep)
{
  listElem_t * newElem = NULL;
  void * newElemValue = NULL;

  if (NULL == list || NULL == valuep) {
    errno = EINVAL;
    return -1;
  }


  if ( -1 == pthread_mutex_lock(&list->mtx) )
    return -1;

  while (list->maxSize != 0  &&
	 list->size == list->maxSize) /* list full */
  {
    errno = 0;
    pthread_cond_wait(&list->monitor, &list->mtx);
    if (EINTR == errno) {
      pthread_mutex_unlock(&list->mtx);
      return -1;
    }
  }      

  if ( NULL == (newElemValue = list->copy_fun(valuep)) ) {
    pthread_mutex_unlock(&list->mtx);
    return -1;
  }
     
  if ( NULL == (newElem = malloc(sizeof(listElem_t))) ) {
    free(newElemValue);
    pthread_mutex_unlock(&list->mtx);
    return -1;
  }

  newElem->value = newElemValue;
  newElem->next = NULL;

  if (NULL == list->head)
    list->head = newElem;
  else {
    listElem_t * itr = list->head;
    while (NULL != itr->next)
      itr = itr->next;
    itr->next = newElem;
  }
   
  list->size++;

  pthread_cond_signal(&list->monitor); /* list isn't empty */

  if ( -1 == pthread_mutex_unlock(&list->mtx) )
    return -1;


  return 0;
}

int 
blockingList_take(blockingList_t * list, void ** dest)
{
  listElem_t * toRmv = NULL;
  listElem_t * prvs = list->head;

  if (NULL == list || NULL == dest) {
    errno = EINVAL;
    return -1;
  }


  if ( -1 == pthread_mutex_lock(&list->mtx) )
    return -1;

  while (!list->head)		/* list empty */
  {
    pthread_cond_wait(&list->monitor, &list->mtx);
    if (EINTR == errno) {
      pthread_mutex_unlock(&list->mtx);
      return -1;
    }
  }

  if (!prvs->next) {
    list->head = NULL;
    toRmv = prvs;
  } else {
    while ( prvs->next->next ) prvs = prvs->next;
    toRmv = prvs->next;
  }

  prvs->next = NULL;
  if (dest) {
    *dest = toRmv->value;
  } else {
    free(toRmv->value);
    toRmv->value = NULL;
  }
  free(toRmv);

  pthread_cond_signal(&list->monitor); /* list not full */

  if ( -1 == pthread_mutex_unlock(&list->mtx) )
    return -1;
    
  return 0;
}

int 
blockingList_remove(blockingList_t * list, void * valuep)
{
  listElem_t * toRmv = NULL;

  if (NULL == list || NULL == valuep) {
    errno = EINVAL;
    return -1;
  }


  if ( -1 == pthread_mutex_lock(&list->mtx) )
    return -1;

  while (!list->head)		/* list empty */
  {
    pthread_cond_wait(&list->monitor, &list->mtx);
    if (EINTR == errno) {
      pthread_mutex_unlock(&list->mtx);
      return -1;
    }
  }
      

  if (0 == list->cmpr_fun(list->head->value, valuep)) {
    toRmv = list->head;
    list->head = toRmv->next;
    free(toRmv->value);
    free(toRmv);
    list->size--;
  } else {
    listElem_t * itr = list->head;
    while (itr->next) {
      if (0 == list->cmpr_fun(itr->next->value, valuep)) {
	toRmv = itr->next;
	itr->next = toRmv->next;
	free(toRmv->value);
	free(toRmv);
	list->size--;
	break;
      }
      itr = itr->next;
    }
  }

  pthread_cond_signal(&list->monitor); /* list not full */

  if ( -1 == pthread_mutex_unlock(&list->mtx) )
    return -1;
    
  return 0;
}

int 
blockingList_removeAll(blockingList_t * list, void * valuep) /* TODO */
{
  return 0;
}

int 
blockingList_contains(blockingList_t * list, void * valuep)
{
  listElem_t * itr = NULL;
  int found = 0;

  if (NULL == list || NULL == valuep) {
    errno = EINVAL;
    return -1;
  }

  
  if ( -1 == pthread_mutex_lock(&list->mtx) )
    return -1;


  itr = list->head;

  while (itr  &&  !found) {
    if (!list->cmpr_fun(itr->value, valuep))
      found = 1;
    itr = itr->next;
  }


  if ( -1 == pthread_mutex_unlock(&list->mtx) )
    return -1;


  return found;
}

int
blockingList_clear(blockingList_t * list)
{
  listElem_t * itr = NULL;

  if (NULL == list)
    return -1;


  if ( -1 == pthread_mutex_lock(&list->mtx) )
    return -1;
  
  
  itr = list->head;

  while (itr) {
    listElem_t * toRmv = itr;
    itr = itr->next;
    _listElem_free(toRmv);
  }

  if ( -1 == pthread_mutex_unlock(&list->mtx) )
    return -1;
  

  return 0;
}

int 
blockingList_getSize(blockingList_t * list)
{
  if (NULL == list) {
    errno = EINVAL;
    return -1;
  }

  return list->size;
}

int 
blockingList_getMaxSize(blockingList_t * list)
{
  if (NULL == list) {
    errno = EINVAL;
    return -1;
  }

  return list->maxSize;
}

void * 
blockingList_peek(blockingList_t * list)
{
  if (NULL== list) {
    errno = EINVAL;
    return NULL;
  }

  return list->head->value;
}
