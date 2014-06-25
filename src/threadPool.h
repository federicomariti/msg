/**
 *  \file threadPool.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "threadPool_task.h"
#include "taskResult.h"


struct threadPool_t;
typedef struct threadPool_t threadPool_t;


/*
 *   stati in cui si puo' trovare la struttura threadPool_t 
 */

/** la  struttura  e'  attiva  e  pronta  per  ricevere  richieste  di
 *  sottomissione  
 */
#define TDPLS_RUN            0  	

/** la  struttura fermata,  non  vengono accettate  alte richieste  di
 *  sottomissione, vengono esauriti i task attivi e quelli in coda
 */
#define TDPLS_STOP_DOALL     1

/** la struttura  fermata.  non  vengono accettate altre  richieste di
 *  sottomissione, vengono esauriti solo i task attivi, i task in coda
 *  sono ignorati.
 */
#define TDPLS_STOP_DOACTIVE  2

/** la struttura e' fermata,  non vengono accettate altre richieste di
 *  sottomissione, si cerca di interrompere tutti i thread del pool
 */
#define TDPLS_STOP_NOW       3

/** la struttura non e' attiva, le risorse occupate sono state liberate
 */
#define TDPLS_TERMINATED     4	


/*
 *   funzioni su una struttura threadPool_t
 */


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
threadPool_t * threadPool_new(int minSize, 
			      int maxSize,
			      int idleTime);

threadPool_t * threadPool_newFixed(int minSize, 
				   int maxSize); 

threadPool_t * threadPool_newCached(int idleTime); 

/** richiede  la sottomissione  di una  attivita' asincrona  al thread
 *  pool.  tale attivita'  verra' esegita  in un  qualche  istante nel
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
 */
int threadPool_submit(threadPool_t * tpool,
		      void *      (* taskFun) (void *),
		      void *         taskArg,
		      taskResult_t * taskResult,
		      void *         returnErrorValue);

int threadPool_submitTask(threadPool_t *       tpool,
			  threadPool_task_t *  task);

/** non si accettano altre sottomissioni, si eseguono tutti i task
 *  attivi e tutti quelli in coda. Le risorse occupate dal thread pool
 *  vengono liberate solo con la successiva invocazione di
 *  threadPool_awaitTermination
 *
 */
int threadPool_shutdown(threadPool_t * tpool);

/** non si accettano altre sottomissioni, si eseguono tutti i task
 *  attivi, tutti i task in coda non ancora avviati vengono ignorati.
 *  Le risorse occupate dal thread pool vengono liberate solo con la
 *  successiva invocazione di threadPool_awaitTermination
 *
 */
int threadPool_shutdownDoActive(threadPool_t * tpool);

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
int threadPool_shutdownNow(threadPool_t * tpool);

/** viene inviato il segnale sig a tutti i thread attivi del pool
 *
 *  \param  tpool  il riferimento al thread pool
 *
 *  \param  sig    il segnale
 *
 *  \retval  0     tutto e' andato bene
 *
 *  \retval  -1    si e verificato un errore, variabile errno settata
 *
 */
int threadPool_killAll(threadPool_t * tpool, int sig);

/** deve essere invocato dopo threadPool_shutdown o
 *  threadPool_shutdownNow, blocca il thread che l'ha invocato fino
 *  alla terminazione di tutti i task e alla distruzione del thread
 *  pool 
 *
 */
int threadPool_awaitTermination(threadPool_t * tpool);

/** ritorna 1 (vero) se questo thread pool si sta' spegnendo
 *
 *  \param  tpool  il thread pool
 *
 *  \retval  1  il thread pool si sta' spegnendo
 *
 *  \retval  0  altrimenti
 *
 *  \retval  -1 si e' verificato un errore, errno settato
 *
 */
int threadPool_isShutdown(threadPool_t * tpool);

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
 *  \retval  -1 si e' verificato un errore, errno settato
 *
 */
int threadPool_isTerminated(threadPool_t * tpool);
  

#endif /* __THREADPOOL_H__ */
