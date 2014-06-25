/**
 *  \file threadPool_thread.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef __THREADPOOL_THREAD_H__
#define __THREADPOOL_THREAD_H__

#include <pthread.h>
#include "threadPool_thread.h"
#include "threadPool_task.h"

/** struttura che rappresenta un thread all'intrno di un thread pool,
 *  sara' quindi il valore di un elemento della lista di thread del
 *  thread pool.
 *
 */
struct threadPool_thread_t {
  pthread_t            thread;
  char                 status;
  threadPool_task_t *  todo;

};

#define TDPLTS_ACTIVE     0
#define TDPLTS_IDLE       1
#define TDPLTS_DESTROYED  2


typedef struct threadPool_thread_t threadPool_thread_t;

/** alloca in memoria heap la struttura che identifica un thraed del
 *  pool e la inizializza: crea un pthread e imposta lo stato della
 *  struttura a "non attivo"
 *
 *  \param attr          riferimento agli eventuali attributi del task
 *                       del pthread (NULL se non esitono)
 *
 *  \param task_routine  riferimento alla funzione del task del
 *                       pthread
 *
 *  \param arg           riferimento agli eventuali argomenti del task
 *                       del pthread (NULL se non esistono)
 *
 *  \return riferimento alla struttura threadPool_thread_t allocata in
 *          memoria heap ed inizializzata
 *
 *  \retval NULL   se si e' verificato un errore, errno settato.
 *
 */
threadPool_thread_t * 
threadPool_thread_new(pthread_attr_t * pthr_attr,
		      void * (* task_routine) (void *), 
		      void * arg);

/** libera tutte le risorse occupate da un threadPool_thread_t
 *
 *  \param thread    riferimento alla struttura che descrive il thread
 *
 */
void
threadPool_thread_destroy(threadPool_thread_t * thread);


/** dealloca   la    memoria   heap   occupata    da   una   struttura
 *  threadPool_thread_t  e da  tutte  le strutture  riferite nei  suoi
 *  campi. se  il pthread associato  a tale struttura e'  attivo viene
 *  invocata la funzione pthread_cancel sul pthread stesso.
 *
 *  \param thread   riferimento al puntatore della struttura che
 *                  descrive il thread
 *
 */
void 
threadPool_thread_free(threadPool_thread_t ** thread);

/** funzione di copia di strutture threadPool_thread_t utilizzata
 *  dalla lista pool di un thread pool: 
 *  una volta creto un thread questo non viene copiato nella lista ma
 *  riferito 
 *
 */
void *
threadPool_thread_copy(void * t);

/** funzone di comparazione di struttre threadPool_thread_t utilizzata
 *  dalla lista pool di un thread pool:
 *  due thread del pool sono uguali se i rispettivi pthread sono
 *  uguali
 *
 */
int
threadPool_thread_compare(void * t1, void * t2);


#endif  /* __THREADPOOL_THREAD_H__ */
