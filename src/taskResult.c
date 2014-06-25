/**
 *  \file taskResult.c
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <stdlib.h>
#include <errno.h>

#include "taskResult.h"
#include "taskResult_private.h"


/** inizializza la struttura che rappresenta il risultato pendente di
 *  una attivita' asincrona
 *
 *  \param  tresult  riferimento alla struttura taskResult_t
 *
 *  \retval  0   tutto e' andato bene
 *
 *  \retval  -1  si e' verificato un errore, errno settato
 *
 */
int 
taskResult_init(taskResult_t * tresult)
{
  if (NULL == tresult) {
    errno = EINVAL;
    return -1;
  }


  tresult->result = NULL;

  tresult->status = TKRTS_NOTDONE;

  if (-1 == pthread_mutex_init(&tresult->mtx, NULL)) return -1;

  if (-1 == pthread_cond_init(&tresult->monitor, NULL)) return -1;
  

  return 0;
}


/** dealloca la memoria heap alloacata dal taskResult_init
 *
 *  \param tresult  riferimento alla struttura che rappresenta il
 *                  risultato dell'attivita' asincrona
 *
 *  \retval  0  tutto e' andato bene
 *
 *  \retval  -1 si e' verificato un errore
 *
 */
int 
taskResult_destroy(taskResult_t * tresult)
{
  if (NULL == tresult) {
    errno = EINVAL;
    return -1;
  }


  if (-1 == pthread_mutex_lock(&tresult->mtx)) return -1;

  tresult->status = TKRTS_DESTROYED; /* OBSOLETO */

  if (-1 == pthread_mutex_unlock(&tresult->mtx)) return -1;
  
  if (-1 == pthread_mutex_destroy(&tresult->mtx)) return -1;

  if (-1 == pthread_cond_destroy(&tresult->monitor)) return -1;
  

  return 0;
}


/** alloca ed inizializza una struttura che rappresenta il risultato
 *  pendente di una attivita' asincrona
 *
 *  \return il riferimento alla nuova struttura
 *
 *  \retval NULL  se si e' verificato un errore, errno settato
 *
 */
taskResult_t * 
taskResult_new(void)		
{
  taskResult_t * result = NULL;

  if ( NULL == (result = malloc(sizeof(taskResult_t))) )
    return NULL;

  if ( -1 == taskResult_init(result) )
    return NULL;

  return result;
}


/** libera tutte le risorse occupate dalla struttura taskResult_t
 *  passata e delloca la memoria usata dalla struttura
 *
 */
void
taskResult_free(taskResult_t ** tresult) 
{
  if (NULL == tresult || NULL == *tresult)
    return;

  if ( 0 == taskResult_destroy(*tresult) )
    free(*tresult);

  return;
}


/** verifica  se l'attivita'  asincrona rappresentata  dalla struttura
 *  taskResult_t passata e' completata.
 *
 *  un task non  e' completato se non e' ancora stato  avviato o se e'
 *  attivo, un task e' completato  se e' terminato normalmente o se e'
 *  terminato in modo  anomalo (si e' verificato un  errore nel codice
 *  del task) o se il thread che lo eseguiva e' stato cancellato.
 * 
 *  \param  tresult  riferimento alla struttura che rappresenta il
 *          risultato di un task
 *
 *  \rval  0   il task non e' terminato
 *  \rval  1   il task e' terminato
 *  \rval  -1  si e' verificato un errore, errno settato
 *
 */
int
taskResult_isDone(taskResult_t * tresult)
{ 
  if (NULL == tresult || TKRTS_DESTROYED == tresult->status) {
    errno = EINVAL;
    return -1;
  }
    
  return !TKRTS_NOTDONE == tresult->status;
}


/** il thread che invoca tale funzione viene sospeso fin quando non e'
 *  disponibile un risutato per il task sepcificato.
 *  il risultato  puo' essere: significativo se lo  stato e' TKRTS_DONE,
 *  altrimenti non significativo se lo stato e' TKRTS_ERROR.
 *  il risultato e  lo stato del task sono settati  da un altro thread
 *  che  esegue  il  task  e  che  invoca  taskResult_setResult,  tale
 *  funzione  sblocca   anche  tutti  i  thread   che  hanno  invocato
 *  taskResult_getResult   e   che   quindi   sono   in   attesa   del
 *  risultato/terminzione del task.
 *
 *  \param  tresult   riferimento  alla  struttura   taskResult_t  che
 *                    rappresenta   il   risultato   pendente   dell'
 *                    esecuzione di un task
 *
 *  \param  result riferimento all'area  di memoria che  contterra' il
 *                    valore di ritorno  del task, se l'esecuzione del
 *                    task e' avvenuta correttamente, NULL altrimenti
 *
 *  \retval 0    se il task e' stato eseguito correttamente, il
 *               risultato salvato in  *result  e' significativo
 *
 *  \retval -1   se si e' verificato un errore, errno settato
 *
 *  \retval -2   se durante l'esecuzione del task si verifica un
 *               errore che ha portato alla terminzione precoce del
 *               task stesso. 
 *               nota che se l'errore e' identificato da un valore
 *               di errno, tale valore e' salvato in  *result
 *
 */
int
taskResult_getResult(taskResult_t * tresult, void ** result)
{
  if (NULL == tresult || TKRTS_DESTROYED == tresult->status ||
      NULL == result) {
    errno = EINVAL;
    return -1;
  }

  
  if (TKRTS_NOTDONE != tresult->status) {
    *result = tresult->result;
    switch (tresult->status) {
    case TKRTS_DONE:      return 0;
    case TKRTS_ERROR:     return -2;
    case TKRTS_CANCELLED: return -3;
    }
  } 
    

  if (-1 == pthread_mutex_lock(&tresult->mtx)) return -1;

  while (TKRTS_NOTDONE == tresult->status)
    pthread_cond_wait(&tresult->monitor, &tresult->mtx);

  if (-1 == pthread_mutex_unlock(&tresult->mtx)) return -1;
    

  *result = tresult->result;
  switch (tresult->status) {
  case TKRTS_DONE:      return 0;
  case TKRTS_ERROR:     return -2;
  case TKRTS_CANCELLED: return -3;
  }
  return -1;
}


/** vine impostato il risultato e lo stato di terminazione di un task
 *  nella struttura riferita da tresult
 * 
 *  \param  tresult     riferimento alla struttura taskResult_t
 *
 *  \param  result      valore ritornato dalla funzione del task in
 *                      caso di terminazione corretta o codice errno.h
 *                      in caso di errore
 *
 *  \param exitStatus  codice dello stato  di terminazione, TKRTS_DONE
 *                      se   la   terminazione   e'  stata   corretta,
 *                      TKRTS_ERROR  se  invece  si e'  verificato  un
 *                      errore
 *
 */
int
taskResult_setResult(taskResult_t * tresult, void * result, char exitStatus)
{
  if (NULL == tresult || TKRTS_DESTROYED == tresult->status ||
      (TKRTS_NOTDONE   != exitStatus && 
       TKRTS_DONE      != exitStatus &&
       TKRTS_ERROR     != exitStatus &&
       TKRTS_CANCELLED != exitStatus   )) {
    errno = EINVAL;
    return -1;
  }

  if (-1 == pthread_mutex_lock(&tresult->mtx)) return -1;

  /* imposta il risultato */
  tresult->result = result;	

  /* imposta lo stato di uscita */
  tresult->status = exitStatus;	

  /* sveglia tutti i task bloccati in attesa del risultato */
  pthread_cond_broadcast(&tresult->monitor);

  if (-1 == pthread_mutex_unlock(&tresult->mtx)) return -1;


  return 0;
}
