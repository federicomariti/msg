/**
 *  \file msgserv.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef __MSGSERV_H__
#define __MSGSERV_H__

#include "genHash.h"
#include "blockingList.h"
#include "comsock.h"

#define Free(refvar) \
  free((refvar));    \
  (refvar) = NULL;


/* stringhe usate nel campo buffer per i mesaggi di tipo MSG_ERROR */
#define _comerr_userNameEmpty_string    "User name is empty"
#define _comerr_wrongMsgType_string     "Wrong messagge tyoe"
#define _comerr_msgTypeNotConn_string   "Message type must is 'connect'"
#define _comerr_userNotAutor_string     "User is not authorized"
#define _comerr_userAlreadyConn_string  "User is already connect"
#define _comerr_closeConnection_string  "Unexpected error, connection closed"
#define _comerr_genericError_string     "Unexpected error, try again"
#define _comerr_serverClosing_string    "Server shutdown"
#define _comerr_oneRcverNotAuth_string  "utente non connesso"/*"Unknown receiver name"*/
#define _comerr_oneRcverNotConn_string  "utente non connesso" /*"Reciver not logged"*/


/* tipo di mesaggio usato da un thread ricevente per indicare al
   rispettivo thread mittente di terminare. */
#define MSG_exit 'x'


/* ---- v userStatus v -------------------------------------------------- */

#define USRSTAT_CONNECTED  'C'
#define USRSTAT_OFFLINE    'O'

/* struttura che rappresenta lo stato di un utente autorizzato per
 * loggarsi in msgserv, contine la lista dei messaggi in uscita per
 * l'utente nel caso in cui l'utente sia connesso al server.
 */
typedef struct {
  char status;
  blockingList_t * outMsg;

} userStatus_t;

#define initOfflineUser(userStatusVar)    \
  userStatusVar.status = USRSTAT_OFFLINE; \
  userStatusVar.outMsg = NULL;

void * userStatus_copy(void * a);
int userStatus_compare(void * a, void * b);

/* ---- ^ userStatus ^ -------------------------------------------------- */

/*
 * prototipi sulla tabella utenti autorizzati 
 */

/** aggiunge un utente alla tabella hash degli utenti autorizzati */
int              userTable_add(hashTable_t *, char *, 
			       userStatus_t *);

/** ricerca un utente nella tabella hash degli utenti autorizzati */
userStatus_t *   userTable_find(hashTable_t *, char *);

/** imposta lo stato di un utente nella  tabella hash degli utenti autorizzati */
int              userTable_set(hashTable_t *, char *, 
			       userStatus_t *, 
			       userStatus_t **);

/** blocca il mutex associato logicamente alla tabella degli utenti **/
#define userTable_lock() userTable_mutex(1);

/** sbblocca il mutex associato logicamente alla tabella degli utenti **/
#define userTable_unlock() userTable_mutex(0);

/** distrugge il mutex associato logicamente alla tabella degli utenti **/
#define userTable_destroyLock() userTable_mutex(-1);

/** incorpora un pthread_mutex_t associato "logicamente" alla tabella
 *  hash degli utenti, offere funzionalita' di lock, unlock e destroy
 *  sul pthread_mutex_t stesso, i comportamenti di tali funzionalita'
 *  sono quelli delle funzioni della libreria pthread con in aggiunta
 *  il controllo di errori senza pero' usare caratteristiche
 *  aggiuntive di pthread_mutex_t non portabili (es. il tipo
 *  PTHREAD_MUTEX_ERRORCHECK)
 *
 *  \param dolock  maggiore di zero, tenta di bloccare il mutex, pari a
 *                 zero, prova a sbloccare il mutex, minore di zero
 *                 distrugge il mutex
 *
 *  \retval  0      tutto e' andato bene
 *
 *  \retval -1      si e' verificato un errore, variabile errno
 *                  settata
 *                  dolock >  0 [EDEADLK] mutex gia' bloccato dal
 *                                        thread corrente
 *                  dolock == 0 [EPERM] mutex bloccato da un altro
 *                                      thread 
 *                  dolock <  0 [EPERM] mutex bloccato da un altro
 *                                      thread 
 *                              [EINVAL] mutex non inizializzato
 *
 */
int userTable_mutex(int dolock);

/*
 * prototipi sulla lista (LIFO) dei messaggi di log da scrivere 
 */
int pushLogMessage(blockingList_t *, char *);  /* put in head */
int popLogMessage(blockingList_t *, char **);  /* take from head */

/*
 * prototipi sulla lista (FIFO) dei messaggi in uscita verso una certa
 * connessione 
 */
int putMessage(blockingList_t *, message_t *);    /* put in tail */
int takeMessage(blockingList_t *, message_t **);  /* take from head */


/*
 * prototipo dei task eseguiti dai threads del processo msgserv 
 */
void * workerReceiverTask(void * arg);
void * workerSenderTask(void * arg);
void * writerTask(void * arg);

#endif /* __MSGSERV_H__ */
