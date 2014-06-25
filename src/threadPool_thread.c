/**
 *  \file threadPool_thread.c
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <stdlib.h>
#include <errno.h>

#include "threadPool_thread.h"

/** alloca in memoria heap la struttura che identifica un thread di un
 *  thread-pool. tale struttura contiene un pthread che verra' creato
 *  con i parametri, la ruotine e gli argomenti passati.
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
		      void * arg)
{
  threadPool_thread_t * result = NULL;
  int err = 0;
  void ** Arg = NULL;
  

  /* alloco la struttura per identificarte un thread del pool */
  if ( NULL == (result = malloc(sizeof(threadPool_thread_t))) ) 
    return NULL;

  /* inizializzo lo stato del thread */
  result->status = TDPLTS_IDLE;

  /* inizializzo il task corrente a non definito */
  result->todo = NULL;

  /* alloco la memoria heap per il passaggio dei parametri al task
     eseguito dal thread del pool */
  if( NULL == (Arg = malloc(sizeof(void *) * 2)) )
    return NULL;

  /* inizializzo gli argomenti passati al task del thread del pool */
  Arg[0] = (void *) arg;    /* riferimento alla struttura del thread
			       pool */
  Arg[1] = (void *) result; /* riferimento alla struttura del thread
			       del pool appena creato */
  
  /* inizializzo ed avvio il thread */
  if ( 0 != ( err = pthread_create(&result->thread, pthr_attr,
				   task_routine, Arg) ) ) {
    errno = err;
    return NULL;
  }    


  return result;
}

/** libera tutte le risorse occupate da un threadPool_thread_t
 *
 *  \param thread    riferimento alla struttura che descrive il thread
 *
 */
void
threadPool_thread_destroy(threadPool_thread_t * thread)
{
  if ( NULL == thread )
    return ;

  free(thread->todo);
}

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
threadPool_thread_free(threadPool_thread_t ** thread)
{
  if (NULL == thread) return;

  /* libero la memoria occupata dalla struttura threadPool_task_t
     associata a thread */
  threadPool_task_free(&(*thread)->todo);

  pthread_cancel((*thread)->thread);

  /* libero la memoria occupata dalla struttura threadPool_thread_t */ 
  free(*thread);

  *thread = NULL;
}


/** funzione di copia di strutture threadPool_thread_t utilizzata
 *  dalla lista pool di un thread pool: 
 *  una volta creto un thread questo non viene copiato nella lista ma
 *  riferito 
 *
 */
void *
threadPool_thread_copy(void * t)
{
  return t;
}

/** funzone di comparazione di struttre threadPool_thread_t utilizzata
 *  dalla lista pool di un thread pool:
 *  due thread del pool sono uguali se i rispettivi pthread sono
 *  uguali
 *
 */
int
threadPool_thread_compare(void * t1, void * t2)
{
  int result = (!pthread_equal(* (pthread_t *) t1,
			       * (pthread_t *) t2) != 0);

  return result;
}

