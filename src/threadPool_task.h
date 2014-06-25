/**
 *  \file threadPool_task.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef __THREADPOOL_TASK_H__
#define __THREADPOOL_TASK_H__

#include "taskResult.h"

struct threadPool_task_t;
typedef struct threadPool_task_t threadPool_task_t;

/** alloca in memori heap la struttura che rappresenta un task, ovvero
 *  una attivita' asincrona, di un thread pool.
 *
 *  \param  fun    il riferimento alla funzione che definisce
 *                 l'attivita' asincrona
 *
 *  \param  arg    il riferimento algi eventuali argomenti
 *
 *  \param  rslt   il riferimento alla struttura ch rappresenta il
 *                 futuro risultato dell'attivita' asincrona
 *
 *  \param returnErrVal   il valore ritornato dalla funzione riferita
 *                        da fun che rappresenta una esecuzione non
 *                        corretta  dell'attivita' asincrona
 *
 */
threadPool_task_t * threadPool_task_new(void * (* fun) (void *),
					void * arg, 
					taskResult_t * rslt, 
					void * returnErrVal); 

/** libera tutte le risorse occupate dalla struttura passata e
 *  dealloca la memoria heap usata dalla struttura, mette a null il
 *  riferimento alla struttura
 *
 */
void threadPool_task_free(threadPool_task_t ** task);


/** inizializza i campi della struttura threadPool_task_t passata e
 *  gia' allocata 
 *
 */
int threadPool_task_init(threadPool_task_t * task,
			 void * (* fun) (void *), void * arg, 
			 taskResult_t * rslt, void * rtnErrVl); 


/** funzione che crea una copia in memoria heap della struttura
 *  threadPool_task_t passata
 *
 *  \param  a   il riferimento alla struttura threadPool_task_t da
 *              copiare 
 *
 *  \return  il riferimento alla locazione di memoria heap in cui e'
 *           stata copiata la nuova struttura
 *
 *  \retval  NULL   in caso di errore, errno settato
 *
 */
void * threadPool_task_copy(void * a);

/** funzione di comparazione tra due strutture threadPool_thread_t, la
 *  comparazione e' effettuata verificaando il valore dei campi delle
 *  due strutture passate
 *
 *  \param  a  il riferimento alla prima struttura threadPool_thread_t
 *
 *  \param  b  il riferimento alla seconda struttura
 *             threadPool_thread_t 
 *
 *  \retval 0  le due strutture sono uguali
 *
 *  \retval -1 altrimenti
 *
 */
int threadPool_task_compare(void * a, void * b);

#endif /* __THREADPOOL_TASK_H__ */
