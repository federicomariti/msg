/**
 *  \file threadPool_private.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef __THREADPOOL_PRIVATE_H__
#define __THREADPOOL_PRIVATE_H__

#include <pthread.h>
#include "blockingList.h"

/** struttura che rappresenta un insieme di thread debiti a servire le
 *  richieste di sottomissione di attivita' asincrone dell'utente.
 *  tali  attivita' sono  caratterizzate  da un  puntatore  a void  in
 *  ingresso e un puntatore a void in uscita.
 *  il risultato  dell'attivita' puo' essere singnificativo  o non. 
 *  il valore in ingresso/uscita  della funzione che definisce il task
 *  puo' essere usato (o meno) dalla funzione stessa per rappresentare
 *  un valore  di ingresso/uscita o  per riferire l'indirizzo  base di
 *  uno  o piu'  valori  in ingresso/usicta.  si  noti che  cio' e'  a
 *  discrezione  dell'utente e  della funzione  che definisce  il task
 *  sottomesso al thread pool.
 *  per memorizzare il risultato e renderlo disponibile all'utente nel
 *  futuro,   offrendo  funzionalita'  di   testing  e   attesa  della
 *  terminazione  del  task, viene  usata  la struttura  taskResult_t,
 *  inizializzata dall'utente e passata  per riferimento con il metodo
 *  submit.
 *  se il  riferimento alla struttura taskResult_t  che rappresenta il
 *  risultato pendente dell'esecuzione di un task e' NULL il risultato
 *  del task  e' ritenuto non  significativo e non viene  salvato.  se
 *  invece  il  risultato  e'  ritenuto  significativo  l'utente  deve
 *  fornire,  oltre  che  alla struttura  taskResult_t  inizializzata,
 *  anche l'unico  valore che identifica  una esecuzione del  task non
 *  corretta   per   poter  impostare   correttamente   lo  stato   di
 *  terminazione del task.
 *
 */
struct threadPool_t {
  blockingList_t * pool;	/** list of (pthread_t *)  */
  char status;			
  int minPoolSize;		
  int threadIdleTime;
  int numOfActiveThread;
  pthread_mutex_t mtx;		/** lock per numOfActiveThread */
  blockingList_t * taskStack; 	/** list of threadPool_task_t */

};


#endif /* __THREADPOOL_PRIVATE_H__ */
