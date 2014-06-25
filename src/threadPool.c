/**
 *  \file threadPool.c
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sched.h>

#include "threadPool.h"
#include "threadPool_private.h"
#include "threadPool_thread.h"
#include "threadPool_task_private.h"
#include "blockingList_itr.h"


#ifdef dbg_threadPool
  static char dbg = 1;
#elif defined dbg_threadPool_
  static char dbg = 2;
#else 
  static char dbg = 0;
#endif

/* ==== vvv defines =============================================== */


define_blockingList_push(list_pushThread, threadPool_thread_t *)

define_blockingList_pop(list_popThread, threadPool_thread_t **)

define_blockingList_remove(list_removeThread, threadPool_thread_t *)

define_blockingList_push(list_pushTask, threadPool_task_t *)

define_blockingList_pop(list_popTask, threadPool_task_t **)
 
define_blockingList_remove(list_removeTask, threadPool_task_t *)


/* ==== vvv private ============================= ^^^ defines ====  */


#define increaseNumOfActiveThread(tpool_p, err_todo)	\
  if (-1 == pthread_mutex_lock(&tpool_p->mtx))		\
  {  err_todo  }					\
  tpool_p->numOfActiveThread++;				\
  if (-1 == pthread_mutex_unlock(&tpool_p->mtx))	\
  {  err_todo  }

#define decreaseNumOfActiveThread(tpool_p, err_todo)	\
  if (-1 == pthread_mutex_lock(&tpool_p->mtx))		\
  {  err_todo  }					\
  tpool_p->numOfActiveThread--;				\
  if (-1 == pthread_mutex_unlock(&tpool_p->mtx))	\
  {  err_todo  }

#define threadPool_thread_setActiveState(tpool, thr, err_todo)    \
  if ( -1 == blockingList_lock(tpool->pool) )			  \
  {  err_todo  }                                                  \
  thr->status = TDPLTS_ACTIVE;                                    \
  if ( -1 == blockingList_unlock(tpool->pool) )			  \
  {  err_todo  }

#define threadPool_thread_setIdleState(tpool, thr, err_todo)      \
  if ( -1 == blockingList_lock(tpool->pool) )			  \
  {  err_todo  }                                                  \
  thr->status = TDPLTS_IDLE;                                      \
  if ( -1 == blockingList_unlock(tpool->pool) )			  \
  {  err_todo  }

void * threadPool_threadTask(void * arg);


/** inizializza la una struttura threadPool_t gia' allocata e le
 *  risore da essa richieste
 *
 *  \param tpool     riferimento al thread pool
 *
 *  \param minSize   dimensione minima del pool di thread, maggiore o
 *                   uguale di 0
 *
 *  \param maxSize   dimensione massima del pool di thread, maggiore di
 *                   0, uguale a 0 se illimitata
 *
 *  \param idleTime  tempo in cui un thread del pool puo' stare
 *                   inattivo in attesa di un nuovo task da
 *                   eseguire. se pari a 0 allore e' infinito,
 *                   altrimenti quando e' trascorso tale tempo e la
 *                   dimensione del pool e' maggiore di minSize il
 *                   thread termina.
 *  
 *  \retval 0  tutto e' andato bene
 *
 *  \retval -1 si e' verificato un errore, errno settato
 *
 */
int 
threadPool_init(threadPool_t * tpool, int minSize, int maxSize,
		int idleTime)
{
  int i = 0;
  pthread_attr_t pthr_attr;


  if (NULL == tpool || 0 > minSize || 0 > maxSize || 0 > idleTime || 
      minSize > maxSize)
  {
    errno = EINVAL;
    return -1;
  }

  if ( NULL == (tpool->pool = 
		blockingList_new(maxSize, threadPool_thread_copy,
				 threadPool_thread_compare)) )
  {
    return -1;
  }

  if ( NULL == (tpool->taskStack =  
		blockingList_new(0, threadPool_task_copy,
				 threadPool_task_compare)) ) 
  {
    int err = (int) errno;
    blockingList_free(&tpool->pool);
    errno = err;
    return -1;
  }

  tpool->status = TDPLS_RUN;

  tpool->minPoolSize = minSize;

  tpool->numOfActiveThread = minSize;

  pthread_mutex_init(&tpool->mtx, NULL);

  
  if ( 0 != (i = pthread_attr_init(&pthr_attr)) ) {
    errno = i;
    return -1;
  }

  if ( 0 != (i = pthread_attr_setdetachstate(&pthr_attr, 
					     PTHREAD_CREATE_DETACHED)) ) {
    errno = i;
    return -1;
  }


  for (i=0; i<minSize; i++) 
  {
    threadPool_thread_t * next = NULL;

    if ( NULL == (next = threadPool_thread_new(&pthr_attr,
					       threadPool_threadTask, 
					       (void *) tpool)) ) {
      int err = (int) errno;
      threadPool_shutdownNow(tpool);
      errno = err;
      return -1;
    }

    if ( -1 == list_pushThread(tpool->pool, next) ) {
      int err = (int) errno;
      threadPool_shutdownNow(tpool); 
      errno = err;
      return -1;
    }
  }

  return 0;
}

int 
threadPool_initFixed(threadPool_t * tpool,
	        	        int minSize, 
			        int maxSize)
{
  return threadPool_init(tpool, minSize, maxSize, 0);
}


int 
threadPool_initCached(threadPool_t * tpool,
			         int idleTime)
{
  return threadPool_init(tpool, 0, 0, idleTime);
}


/* ==== vvv thread task ========================== ^^^ private ==== */


/** funzione che descrive il task che fa terminare il thread corrente,
 *  usata per implementare la terminazione graduale di un thread pool
 *
 */
void *
threadPool_thread_exitTask(void * arg)
{
  pthread_exit((void *) 0);

  return NULL;
}

void
threadPool_threadTask_cleanup(void * arg)
{
  /* riferimento alla struttura del thread pool */
  threadPool_t * tpool = (threadPool_t *) ((void **) arg) [0];

  /* riferimento alla struttura che descrive il therad all'interno del
     thread pool */
  threadPool_thread_t * thr = (threadPool_thread_t *) ((void **) arg) [1];
  
  if (dbg) fprintf(stderr, "---DBG--- TDPL-worker-%d: cleanup start\n", 
		   (int)pthread_self());

  /* nel caso in cui il thread sia stato cancellato durante l'attesa
     di un task da eseguire, ovvero il verificarsi della condizione
     "lista dei task non vuota", il mutex non e' stato rilasciato */
  blockingList_unlock(tpool->taskStack); 

  /* nel caso in cui il thread sia stato cancellato durante
     l'esecuzione di un task */
  if (TDPLTS_ACTIVE == thr->status) 
  {
    decreaseNumOfActiveThread(tpool, ;);
    /* free(thr->todo); lo faccio in ogni caso */
  }

  threadPool_thread_destroy(thr);

  /* libera la memoria occupata dagli argomenti passati a tale
     funzione */
  free(arg);
  
  if (dbg) fprintf(stderr, "---DBG--- TDPL-worker-%d: cleanup end\n",
		   (int)pthread_self());

  /* rimuove il thread corrente dalla lista dei thread del thread pool
     ovvero viene liberata la memoria occupata dalla struttura
     threadPool_thread_t corrispondente. nota che la memoria occupata
     dal descrittore e dallo stack del pthread corrispondente viene
     liberata alla terminazione del pthread stesso, in quanto creato
     detached */
  list_removeThread(tpool->pool, thr);

}

void *
threadPool_threadTask(void * arg)
{
  /* riferimento alla struttura del thread pool */
  threadPool_t * tpool = (threadPool_t *) ((void **) arg) [0];

  /* riferimento alla struttura del thread */
  threadPool_thread_t * thr = (threadPool_thread_t *) ((void **) arg) [1];

  /* riferimento al task da eseguire attualmente prelevato dalla
     lista(pila) dei task del thread pool */
  threadPool_task_t * actualTask = NULL;

  void * actualResult = NULL;

  /* argomento della funzione di cleanup in caso di terminazione del
     thread che esegue tale funzione */
  void * cleanupArg = arg;

  sigset_t defaultSignalMask;


  pthread_cleanup_push(threadPool_threadTask_cleanup, cleanupArg);

  /* salva la signal map del thread di default */
  pthread_sigmask(0, NULL, &defaultSignalMask);

  if (dbg) fprintf(stderr, "---DBG--- TDPL-worker-%d: start\n", (int)pthread_self());
  
  while (TDPLS_RUN        == tpool->status  ||
         TDPLS_STOP_DOALL == tpool->status)
  {
    /*  RIPRISTINA LA SIGNAL MASK DEL THREAD A QUELLA DI DEFAULT */
    pthread_sigmask(SIG_SETMASK, &defaultSignalMask, NULL);

    if (dbg) fprintf(stderr, "---DBG--- TDPL-worker-%d: wait\n", 
		     (int)pthread_self());

    /*
     *  ATTENDI UN TASK
     */
    list_popTask(tpool->taskStack, &actualTask); /* C.P. */ /* error ? */

    if (2==dbg) fprintf(stderr, "---DBG--- TDPL-worker-%d: post wait\n",
		     (int)pthread_self());

    /*  AGGIORNA LO STATO DEL THREAD E DEL THREADPOOL */
    threadPool_thread_setActiveState(tpool, thr, return NULL;)
    increaseNumOfActiveThread(tpool, ;);
    thr->todo = actualTask;

    /* nota: se threadPool_shutdownDoActive e' invocato dopo
     *       list_popTask e prima del lock su tpool->pool la
     *       cancellazione avverrebbe nell'esecuzione del task
     */
    pthread_testcancel();	/* C.P. */

    /*
     *  ESEGUI IL TASK
     */ 
    actualResult = actualTask->function(actualTask->arg); /* ( C.P. ) */  /* PRV */

    /*
     *  SALVA IL RISULTATO
     */
    if (actualTask->result) {	/* PRV <================================= */
      errno = 0;
      if (actualResult == actualTask->returnErrorValue) {
	if (errno)
	  actualResult = (void *)errno;
	taskResult_setResult(actualTask->result,	 /* PRV <======== */
			     actualResult, TKRTS_ERROR); /* error ? */
      } else {
	taskResult_setResult(actualTask->result,	/* PRV <========= */
			     actualResult, TKRTS_DONE); /* error ? */
      }
    } 

    /*  AGGIORNA LO STATO DEL THREAD E DEL THREADPOOL */
    threadPool_thread_setIdleState(tpool, thr, return NULL;);
    decreaseNumOfActiveThread(tpool, ;);

    /* libera la memoria occupata dal descrittore del task eseguito
       nota che il la struttura taskResult_t riferita da un campo di
       threadPool_task_t NON deve essere cancellata */
    free(actualTask);
    thr->todo = NULL;

    pthread_testcancel();	/* C.P. */
    
  }

  if (dbg) fprintf(stderr, "---DBG--- TDPL-worker-%d: terminated\n", (int)pthread_self());

  /*
   *   TERMINAZIONE
   */
  pthread_cleanup_pop(0);
  
  /* rimuovi l'elemento nella lista pool corrispondente al thread
     corrente */
  list_removeThread(tpool->pool, thr);
  
  /* libera la memoria occupata dagli argomenti pasati a tale funzione
     (esguito solo ala terminaizone in quanto tali locazioni di
     memoria sono usate anche per il passaggio dei parametri della
     funzione di clean up) */
  free(arg);


  return  (void *) 0;
}


/* ==== vvv thread pool ====================== ^^^ thread task ==== */


/** alloca in memoria heap una struttura threadPool_t e la inizializza
 *
 *  \param minSize   dimensione minima del pool di thread, maggiore o
 *                   uguale di 0
 *
 *  \param maxSize   dimensione massima del pool di thread, maggiore di
 *                   0, uguale a 0 se illimitata
 *
 *  \param idleTime  tempo in cui un thread del pool puo' stare
 *                   inattivo in attesa di un nuovo task da
 *                   eseguire. se pari a 0 allore e' infinito,
 *                   altrimenti quando e' trascorso tale tempo e la
 *                   dimensione del pool e' maggiore di minSize il
 *                   thread termina.
 *  
 *  \return   il riferimento alla nuova struttura allocata
 *
 *  \retval   NULL   si e' verificato un errore, errno settato
 *
 */
threadPool_t *
threadPool_new(int minSize, int maxSize, int idleTime)
{
  threadPool_t * result = NULL;

  if ( NULL == (result = malloc(sizeof(threadPool_t))) )
    return NULL;

  if ( -1 == threadPool_init(result, minSize, maxSize, idleTime) )
    return NULL;

  return result;
}

threadPool_t * 
threadPool_newFixed(int minSize, int maxSize)
{
  return threadPool_new(minSize, maxSize, 0);
}

threadPool_t *
threadPool_newCached(int idleTime)
{
  return threadPool_new(0, 0, idleTime);
}


int 
__createNewThread(threadPool_t * tpool) 
{
  threadPool_thread_t * next = NULL;
  pthread_attr_t pthr_attr;
  int err = 0;
  void * arg = tpool;
  
  if ( 0 != (err = pthread_attr_init(&pthr_attr)) ) {
    errno = err;
    return -1;
  }

  if ( 0 != (errno = pthread_attr_setdetachstate(&pthr_attr, 
				       PTHREAD_CREATE_DETACHED)) ) {
    return -1;
  }

  if ( NULL == (next = threadPool_thread_new(&pthr_attr,
					     threadPool_threadTask, 
					     (void *) arg)) ) {
    pthread_attr_destroy(&pthr_attr);
    return -1;
  }
 
  if ( -1 == list_pushThread(tpool->pool, next) ) {
    threadPool_thread_free(&next);
    return -1;
  }

  return 0;
}

/** richiede  la sottomissione  di una  attivita' asincrona  al thread
 *  pool.  tale attivita'  verra' esegita  in un  qualche  istante del
 *  futuro    da    un   thread    appartenente    al   thread    pool
 *  specificato. l'attivita'  puo' avere o  meno un valore  di ritorno
 *  significativo, in particolare se  interessa il valore di ritorno e
 *  lo  stato  di esecuzione  dell'attivita',  tali informazioni  sono
 *  rappresentati dalla struttura taskResult_t che deve essere passata
 *  come riferimento.
 *
 *  \param tpool       riferimento del thread pool
 *
 *  \param taskFun     riferimento alla funzione che rappresenta il task
 *                     da sottomettere al thread pool
 *
 *  \param taskArg     riferimento agli arogmenti della funzione taskFun,
 *                     NULL se la funzione non richiede argomenti
 *
 *  \param taskResult  riferimento ad  una struttura  taskResult_t che
 *                     permette   all'utente   del   thread  pool   di
 *                     conoscere  lo stato  di esecuzione  del  task e
 *                     prelevare/attendere  il risultato.  Puo' essere
 *                     null  se  il  task  non ha  valori  di  ritorno
 *                     significativi  o  non  interessa  conoscere  lo
 *                     stato di esecuzione del task
 *
 *  \retval 0   tutto e' andato bene.
 *
 *  \retval -1  si e' verificato un errore, errno settato.
 *
 *  \retval -2  il thread pool non e' in esecuzione
 *
 */
int 
threadPool_submit(threadPool_t * tpool,
		  void *      (* taskFun) (void *),
		  void *         taskArg,
		  taskResult_t * taskResult,
		  void *         returnErrorValue)
{
  threadPool_task_t newTask;	/* PRV meglio un riferimento a heap */
  int maxSize = blockingList_getMaxSize(tpool->pool);
  int size = blockingList_getSize(tpool->pool);

  if (NULL == tpool || NULL == taskFun) {
    errno = EINVAL;
    return -1;
  }

  if ( TDPLS_RUN != tpool->status )
    return -2;

  /* se c'e' necessita' creo ed aggiungo un nuovo thread al pool */

  if ( 0 == maxSize ) {
    if ( size > tpool->numOfActiveThread )
      ; /* do nothing */
    else 
      if ( -1 == __createNewThread(tpool) )
	return -1;
  } else {
    if ( maxSize < size  && size == tpool->numOfActiveThread ) {
      if ( -1 == __createNewThread(tpool) )
	return -1;
    } else 
      ; /* do nothing */
    
  }


  /* creo la struttura per il nuovo task sottomesso e la aggiungo alla
     pila dei task in attesa */

  if ( -1 == threadPool_task_init(&newTask,
  taskFun, taskArg, taskResult, returnErrorValue) ) return -1;

  if ( -1 == list_pushTask(tpool->taskStack, &newTask) )
    return -1;


  return 0;
}

/** non si accettano altre sottomissioni, si eseguono tutti i task
 *  attivi e tutti quelli in coda. una volta completati tutti i task
 *  sottomessi si distrugge il thread pool.
 *
 */
int 
threadPool_shutdown(threadPool_t * tpool)
{
  threadPool_task_t exitTask;
  int i = 0;
  int listSize = 0;

  if (NULL == tpool) {
    errno = EINVAL;
    return -1;
  }

  if ( -1 == pthread_mutex_lock(&tpool->mtx) )
    return -1;

  tpool->status = TDPLS_STOP_DOALL;
  listSize = blockingList_getSize(tpool->pool);

  if ( -1 == pthread_mutex_unlock(&tpool->mtx) )
    return -1;

  threadPool_task_init(&exitTask, threadPool_thread_exitTask, 
		       NULL, NULL, NULL);

  for (i=0; i<listSize; i++) {
    blockingList_add(tpool->taskStack, &exitTask);
  }

  return 0;
}

/** non si accettano altre sottomissioni, si eseguono tutti i task
 *  attivi, tutti i task in coda non ancora avviati vengono ignorati.
 *  Le risorse occupate dal thread pool vengono liberate solo con la
 *  successiva invocazione di threadPool_awaitTermination
 *
 */
int 
threadPool_shutdownDoActive(threadPool_t * tpool)
{
  blockingList_itr_t pool_itr;

  if (NULL == tpool) {
    errno = EINVAL;
    return -1;
  }

  tpool->status = TDPLS_STOP_DOACTIVE;

  /* rimuovi tutti i task in attesa nella lista taskStack */

  blockingList_clear(tpool->taskStack);

  /* cancella(termina) tutti i thread inattivi */

  blockingList_itr_init(&pool_itr, tpool->pool);
									
  blockingList_lock(tpool->pool);					

  do {									
    threadPool_thread_t * next = NULL;					
									
    if ( NULL !=  (next = blockingList_itr_getValue(&pool_itr)) )	
									
      if (TDPLTS_IDLE == next->status) {				

	if (dbg) fprintf(stderr, "---DBG--- TDPL: shutdownDoActive: CANCELLATO %d\n",
			 (int)next->thread);
									
	pthread_cancel(next->thread);					
									
      }						
			
  } while(blockingList_itr_next(&pool_itr));
									
  blockingList_unlock(tpool->pool);

  return 0;
}

/** non si accettano altre sottomissioni, si cerca di interrompere
 *  tutti i thread attivi del pool richiedendone la cancellazione
 *  (invocazione della funzione pthread_cancel su ciascun thread del
 *  pool). Le risorse occupate dal thread pool vengono liberate solo
 *  con la successiva invocazione di awaitTermination
 *
 *  nota: la terminazione e' effettivamente istantanea solo se i task
 *        sottomessi al thread pool hanno punti di cancellazione. 
 *
 *  nota: per la corretta terminazione, i task sottomessi devono
 *        prevedere funzioni di cleanup, aggiunte con
 *        pthread_cleanup_push, che liberino le eventuali risorse
 *        occupate dai task stessi
 *
 */
int 
threadPool_shutdownNow(threadPool_t * tpool)
{
  blockingList_itr_t listItr;

  if (NULL == tpool) {
    errno = EINVAL;
    return -1;
  }

  tpool->status = TDPLS_STOP_NOW;

  if ( -1 == blockingList_itr_init(&listItr, tpool->pool) )
    return -1;

  blockingList_lock(tpool->pool);

  do
  {
    threadPool_thread_t * next = NULL;

    if ( NULL != (next = blockingList_itr_getValue(&listItr)) ) {

      if (dbg) fprintf(stderr, "---DBG--- TDPL: shutdownNow: CANCEL %d\n", 
		       (int)next->thread);
      
      pthread_cancel(next->thread);

    }

  } while (blockingList_itr_next(&listItr));

  blockingList_unlock(tpool->pool);

  return 0;
}    

/** non si accettano altre sottomissioni, si cerca di interrompere
 *  tutti i thread del pool inviando a questi il segnale sig. Le
 *  risorse occupate dal thread pool vengono liberate solo con la
 *  successiva invocazione di awaitTermination
 *
 *  nota: per ottenere la terminazione del thread pool occoorre che il
 *        segnale sig non causi la terminazioe del processo (l'utente
 *        del thread pool deve prevedere un adeguato gestore per sig),
 *        ma ritorni alla system call interrotta.
 *
 *  nota: per ottenere una terminazione istantanea del thread pool
 *        occorre che i task sottomessi al thread pool prevedano una
 *        gestione dell'errore EINTR delle system call interrmopibili
 *        usate tale da terminare subito l'esecuzione del task e
 *        pulire l'ambiente 
 *
 */
int 
threadPool_killAll(threadPool_t * tpool, int sig)
{
  blockingList_itr_t listItr;
  threadPool_thread_t * nextThr = NULL;
  
  if (NULL == tpool) {
    errno = EINVAL;
    return -1;
  }

  if ( -1 == blockingList_itr_init(&listItr, tpool->pool) )
    return -1;

  blockingList_lock(tpool->pool);

  do {

    if ( NULL != (nextThr = blockingList_itr_getValue(&listItr)) ) 
      if ( TDPLTS_ACTIVE == nextThr->status )
	if ( -1 == pthread_kill(nextThr->thread, sig) )
	  return -1;

  } while (blockingList_itr_next(&listItr));

  blockingList_unlock(tpool->pool);
    
  return 0;
}

/** deve essere invocato dopo threadPool_shutdown o
 *  threadPool_shutdownNow, blocca il thread che l'ha invocato fino
 *  alla terminazione di tutti i task e alla distruzione del thread
 *  pool 
 *
 */
int 
threadPool_awaitTermination(threadPool_t * tpool)
{
  if (NULL == tpool  ||
      ( TDPLS_STOP_DOALL    != tpool->status  &&
	TDPLS_STOP_DOACTIVE != tpool->status  &&
	TDPLS_STOP_NOW      != tpool->status  ) )
  {
    errno = EINVAL;
    return -1;
  }
  
  blockingList_lock(tpool->pool);
  while (!blockingList_isEmpty(tpool->pool)) 
  {
    blockingList_wait(tpool->pool);
  }
  blockingList_unlock(tpool->pool);

  
  {
    struct timespec tosleep;

    tosleep.tv_nsec = 0;
    tosleep.tv_sec = 1;
    nanosleep(&tosleep, NULL);
  }


  blockingList_free(&tpool->pool);
  blockingList_free(&tpool->taskStack);

  pthread_mutex_destroy(&tpool->mtx);

  tpool->status = TDPLS_TERMINATED; /* OBSOLETO */

  free(tpool);

  if (dbg) fprintf(stderr, "---DBG--- THRPL: awaitTermination: TERMINATED and DESTROYED\n");

  return 0;
}

/** ritorna 1 (vero) se questo thread pool si sta' spegnendo
 *
 *  \param  tpool  il thread pool
 *
 *  \retval  1  il thread pool si sta' spegnendo
 *
 *  \retval  0  altrimenti
 *
 */
int 
threadPool_isShutdown(threadPool_t * tpool)
{
  if (NULL == tpool)
    return 0;

  return TDPLS_STOP_DOALL == tpool->status  ||
    TDPLS_STOP_DOACTIVE == tpool->status    ||
    TDPLS_STOP_NOW == tpool->status;
}


/** ritorna 1 (vero) se questo thread pool ha eseguito completamente
 *  lo spegnimento, non e' mai vero se prima non e' stato mai invocato
 *  threadPool_shutdown o threadPool_shutdownNow.
 *  nota: tutte le risorse usate dal thread pool sono liberate.
 *
 *  \param  tpool  il thread pool
 *
 *  \retval  1  il thread pool e' terminato
 *
 *  \retval  0  altrimenti
 *
 */
int 
threadPool_isTerminated(threadPool_t * tpool)
{
  if (NULL == tpool)
    return 0;

  return TDPLS_TERMINATED == tpool->status;
}

