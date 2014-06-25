/**
 *  \file blockingList_private.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef __BLOCKINGLIST_PRIVATE_H__
#define __BLOCKINGLIST_PRIVATE_H__

#include <pthread.h>
#include "listElem.h"

/** struttura che descrive una lista bloccante
 *
 */
struct blockingList_t {
  listElem_t * head;

  int size;
  int maxSize;

  void * (* copy_fun) (void *);
  int (* cmpr_fun) (void *, void *);

  pthread_mutex_t mtx;
  pthread_cond_t monitor;
  
};


#endif /* __BLOCKINGLIST_PRIVATE_H__ */
