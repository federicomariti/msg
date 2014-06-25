/**
 *  \file taskResult.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef __TASKRESULT_H__
#define __TASKRESULT_H__

/** rappresenta il risultato pendente di una attivita' asincrona. 
 *
 *  un  qualsiasi  thread  puo'  verificarne lo  stato  di  esecuzione
 *  dell'attivita'   con  taskResult_isDone,   e'   inoltre  possibile
 *  attendere la terminazione dell'attivita' per ritirare il risultato
 *  con taskResult_getResult, in questo caso il thread che ha invocato
 *  tale  funzione  si  blocca  fino  a  che  il  risultato  e'  stato
 *  calcolato.    Il   thread    che    esegue   l'attivita'    chiama
 *  taskResult_setResult al completamento dell'attivita' per impostare
 *  il risultato  e lo stato di completamento  dell'attivita' stessa e
 *  per sbloccare tutti i thread in attesa di tale risultato.
 *
 */
struct taskResult_t;
typedef struct taskResult_t taskResult_t;

/*
 *  stati di completamento di un task 
 */

/** il task non e' terminato 
 */
#define TKRTS_NOTDONE        0	

/** il task e' terminato, il risultato e' significativo 
 */
#define TKRTS_DONE           1	

/** il  task  e'  terminato  in  modo incorretto,  il  risultato  puo'
 *  rappresentare   l'errore   che  si   e'   verificato   o  e'   non
 *  significativo 
 */
#define TKRTS_ERROR          2	

/** il task e' terminato a causa di una richiesta di cancellazione al
 *  thread che lo stava eseguendo
 */
#define TKRTS_CANCELLED      3

/** la struttura che rappresenta il risultato di un task e' stata 
 *  dellocata
 */
#define TKRTS_DESTROYED      4	/* OBSOLETO */


/*
 *   funzioni
 */

/** alloca ed inizializza una struttura che rappresenta il risultato
 *  pendente di una attivita' asincrona
 *
 *  \return il riferimento alla nuova struttura
 *
 *  \retval NULL  se si e' verificato un errore, errno settato
 *
 */
taskResult_t * taskResult_new(void); /* TODO */

/** libera tutte le risorse occupate dalla struttura taskResult_t
 *  passata e delloca la memoria usata dalla struttura. imposta a NULL
 *  il puntatore riferito dal parametro tresult
 *
 *  \param tresult   riferimento al puntatore alla struttura
 *                   taskResult_t da deallocare
 *
 */
void taskResult_free(taskResult_t ** tresult); /* TODO */

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
int taskResult_isDone(taskResult_t * tresult);

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
 *  \retval -2   se durante l'esecuzione del task si e' verificato un 
 *               errore che ha portato alla terminzione precoce del
 *               task stesso. il valore in  *result  potrebbe
 *               rappresentare l'errore, dipende da chi ha impostato
 *               tresult
 *
 *  \retval -3   se il thread che eseguiva il task e' stato terminato
 *               da una cancellazione prima di poter terminare
 *               l'esecuzione corretta del task. il valore  *result e'
 *               non significativo *               
 *
 *
 */
int taskResult_getResult(taskResult_t * tresult, void ** result);

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
int taskResult_setResult(taskResult_t * tresult, void * result, 
			 char exitStatus);

#endif /* __TASKRESULT_H__ */
