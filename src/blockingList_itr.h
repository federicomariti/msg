/**
 *  \file blockingList_itr.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef __BLOCKINGLIST_ITR_H__
#define __BLOCKINGLIST_ITR_H__

#include "blockingList.h"
#include "listElem.h"

typedef struct  {
  blockingList_t *  list;
  listElem_t *      elem;
  unsigned int      index;

} blockingList_itr_t;

/** alloca ed inizializza la struttura di un itreatore in memoria heap
 *  al "primo" elemento della lsita
 *
 */
blockingList_itr_t * blockingList_itr_new(blockingList_t * list); /* TODO */

/** data la struttura di un iteratore gia' allocato, questa viene
 *  inizializzata al "primo" elemento elemento della lista
 *
 */
int blockingList_itr_init(blockingList_itr_t * itr, blockingList_t * list);

/** libera tutte le risorse occupate e la memoria heap occupata
 *  dall'iteratore
 *
 */
void blockingList_itr_free(blockingList_itr_t ** itr); /* TODO */

/** libera tutte le risorse occupate dall'iteratore
 *
 */
int blockingList_itr_destroy(blockingList_itr_t * itr);

/** ritorna il riferimento al valore dell'elmento corrente della lista
 *  iterata. il riferimento al valore e' proprio quello presente nella
 *  lista, NON vine riferita una copia del valore
 *
 */
void * blockingList_itr_getValue(blockingList_itr_t * itr);

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
int blockingList_itr_next(blockingList_itr_t * itr);

/** ritorna vero (valore intero diverso da 0) se l'iteratore ha altri
 *  elementi
 *
 *  \retval !=0  esistono altri elementi nell'iteratore
 *
 *  \retval  0   altrimenti
 *
 */
int blockingList_itr_hasNext(blockingList_itr_t * itr);

int blockingList_itr_remove(blockingList_itr_t * itr); /* TODO */

#endif /* __BLOCKINGLIST_ITR_H__ */
