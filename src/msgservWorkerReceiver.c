/**
 *  \file msgservWorkerReceiver.c
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "msgserv.h"
#include "errorUtil.h"
#include "comsock.h"
#include "genHash.h"
#include "threadPool.h"
#include "blockingList.h"
#include "blockingList_itr.h"

extern char dbg;
extern hashTable_t * usrTable;
extern blockingList_t * logStack;
extern blockingList_t * conntedUsrList;
extern threadPool_t * tpool;

extern volatile sig_atomic_t leave;


/** ritorna il riferimento alla locazione di memoria in cui e' stata
 *  scritta la stringa (terminata da '\0') rappresentatante un messaggio
 *  di log nella seguente forma: "sender:receiver:messageText"
 *
 *  \param  sender      riferimento al nome del mittente
 *
 *  \param  receiver    riferimento al nome del destinatario
 *
 *  \param  message     riferimento al testo del messaggio
 *
 *  \param  senderLng  lunghezza del nome del mittente se 0 tale
 *                     lunghezz viene calcolata 
 *
 *  \retval ref!=NULL  riferimento alla stringa in memoria heap che
 *                     rappresenta il messaggio
 *
 *  \retval ref==NULL  si e' verificato un errore, variabile errno
 *                     settata 
 */
char *
createLogString(const char * sender, const char * receiver, 
		const char * message, int senderLng) 
{
  char * result = NULL;
  int resultLength = 0;

  if ( NULL == sender || NULL == receiver || NULL == message ) {
    errno = EINVAL;
    return NULL;
  }

  if (!senderLng) senderLng = strlen(sender);

  resultLength = senderLng + strlen(receiver) + strlen(message) + 3;

  if ( NULL == (result = malloc(resultLength)) )
    return NULL;
  
  if ( NULL == strcpy(result, sender) 
       || NULL == strcat(result, ":")
       || NULL == strcat(result, receiver)
       || NULL == strcat(result, ":")
       || NULL == strcat(result, message) ) {

    free(result);
    return NULL;
  }

  
  return result;
}

/** ritorna il riferimento alla locazione di memoria in cui e' stata
 *  scritta la stringa (terminata da '\0') rappresentante un messaggio
 *  nella seguente forma: "[sender] messageText"
 *
 *  \param  sender      riferimento al nome del mittente
 *
 *  \param  message     riferimento al testo del messaggio
 *
 *  \param  senderLng  lunghezza del nome del mittente se 0 tale
 *                     lunghezz viene calcolata 
 *
 *  \retval ref!=NULL  riferimento alla stringa (terminata da '\0') in
 *                     memoria heap che rappresenta il messaggio
 *
 *  \retval ref==NULL  si e' verificato un errore, variabile errno
 *                     settata 
 */
char *
createMsgString(const char * sender, const char * message, int senderLng)
{
  char * result = NULL;
  int resultLength = 0;

  if ( NULL == sender || NULL == message) {
    errno = EINTR;
    return NULL;
  }

  if (!senderLng) senderLng = strlen(sender);

  resultLength = senderLng + strlen(message) + 4;

  if ( NULL == (result = malloc(resultLength)) )
    return NULL; /* oom */

  if ( NULL == strcpy(result, "[") 
       || NULL == strcat(result, sender)
       || NULL == strcat(result, "] ")
       || NULL == strcat(result, message) ) {
    
    free(result);
    return NULL;
  }


  return result;
}

/** dato un messaggio ed il riferimento ad un puntatore di caratteri
 * alloca memoria heap dove viene copiato il nome dell'utente
 * destinatario del messaggio stesso. se il riferimento al puntatore
 * di caratteri e' nullo non viene allocata memoria ma viene
 * restituita solo la lunghezza del nome dell'utente destinatario. il
 * messaggio deve essere nella forma: "[receiverName] messageText"
 *
 *  \param  message       riferimento al messaggio
 * 
 *  \param  receiverName  riferimento al puntatore in cui viene
 *                        salvato il riferimento alla stringa
 *                        (terminata da '\0') in  memoria heap del
 *                        nome del destinatario oppure NULL
 *
 *  \retval n>0  la lunghezza del nome utente
 *
 *  \retval -1   si e' verificato un errore, variabile errno settata
 *
 *  \retval -2   il messaggio non e' nel formato giusto
 */
int
getReceiverNameFromMessage(const char * message, char ** receiverName) 
{
  int size = 0, result = 0; 

  if ( NULL == message ) {
    errno = EINVAL;
    return -1;
  }


  if ( '[' == *message   
       || '\0' == *message
       || '\0' == message[1]
       || ']'  == message[1] ) return -2;

  while ( ']' != message[size + 1]
	  && ' '  != message[size + 1]
	  && '\0' != message[size + 1] ) size++;

  if ( ']' != message[size + 1] ) return -2;

  if (receiverName) {
    if ( NULL == (*receiverName = malloc(size)) ) return -1;

    strncpy(*receiverName, message+1, size-1);

    result = 0;
  } else {
    result = size-1;
  }
 
  return size-1;
}


#define _sendLogMsg(logMessageVar, receiverName, messageString, errMsg)  \
  errno = 0;                                                             \
  if ( NULL == (logMessageVar = createLogString(myUsrName, receiverName, \
						messageString,           \
						myUsrName_length)) ) {   \
    if (errno) perror("msgserv: workerTask: createLogString"errMsg);     \
    localLeave = 2;                                                      \
    continue;                                                            \
  }                                                                      \
                                                                         \
  if (2==dbg) fprintf(stderr, "---DBG--- msgserv-rcver: "                \
		      "logMessage = >%s<\n", logMessageVar);             \
                                                                         \
  /* aggiungi nextLogString alla pila del logwriter-thread */            \
  if ( -1 == pushLogMessage(logStack, logMessageVar)) {                  \
    if (errno) perror("workerTask: pushLogMessage"errMsg);               \
    localLeave = 2;                                                      \
    continue;                                                            \
  } 

/*
 *   TASK DEI WORKER THREAD
 */

#define _sendErrorMessageToWriter(_outMsgList, _msgVar, _errStr, _errTodo)  \
  _createErrorMessage(_msgVar, _errStr);   				\
  errno = 0;                                                            \
  if ( -1 == putMessage(_outMsgList, &_msgVar) ) {   	                \
      if (errno) perror("putMessage");				        \
      _errTodo;                                                         \
  }
#define _sendErrorMessageToWriter_(_errStr)                             \
  _sendErrorMessageToWriter(userStatus.outMsg, nextMsgToSnd, _errStr, ;)

#define _sendErrorMessage(_socket_fd, _msgVar, _errStr, _errTodo)       \
  _createErrorMessage(_msgVar, _errStr);   				\
  {                                                                     \
    int __err = 0;                                                      \
    if ( 0 != (__err = sendMessage(_socket_fd, &_msgVar)) ) {  		\
      if ( -1 == __err)							\
	perror("sendMessage");				                \
      else if (SEOF == __err)						\
	; /* do nothing */						\
    }                                                                   \
  }
#define _sendErrorMessage_(_errStr)                                     \
  _sendErrorMessage(sock_fd, nextMsgToSnd, _errStr, ;)



#define _sendMessageToWriter(_outMsgList, _msgVar, _errTodo)            \
  errno = 0;                                                            \
  if ( -1 == putMessage(_outMsgList, &_msgVar) ) {                      \
    if (errno) perror("putMessage");                                    \
    else fprintf(stderr, "putMessage: error\n");                        \
    _errTodo;                                                           \
  }                                                                     
#define _sendMessageToWriter_(_outMsgList, _errTodo)                    \
  _sendMessageToWriter(_outMsgList, nextMsgToSnd, _errTodo)


/* funzione di cleanup per l'esecuzione di workerReceiverTask dopo il
 * login dell'utente e la creazione e aggiornamento delle struttre
 * dati 
 *
 */
void 
workerReceiverThread_cleanup(void * arg) 
{
  char * usrName = (char *) arg;

  userStatus_t offline, * prvsStatus = NULL;

  message_t nextMsgToSnd;


  if (dbg) fprintf(stderr, "---DBG--- msgserv-rcver: cleanup: start\n");

  userTable_lock(); /* non dovrebbe mai succedere(per gestione della
		       cancellazione e errori in workerReceiverTask()
		       in manipolazione tabella utenti), ma nel caso
		       in cui il mutex di userTable e' bloccato dal
		       thread corrente tale funzione lascia il mutex
		       bloccato e ritorna errore, NON causa
		       DEAD-LOCK*/

  /* aggiorna lo stato dell'utente nella tabella degli utenti
   * autorizzati con "non connesso" 
   * nota: la lista dei messaggi in uscita verso tale utente non viene
   *       deallocata ma continua ad essere usata dal thread mittente
   */
  initOfflineUser(offline);

  if ( -1 == userTable_set(usrTable, usrName, &offline, &prvsStatus) ) {
    perror("msgserv-rcver: setUserConn"); 
  }

  userTable_unlock();
    

  /* invia messaggio di terminazione al thread mittente ed
   * eventualmente anche all'utente 
   */
  if ( leave ) { /* se TUTTO il processo msgserv e' terminato */

    /* invia messaggio di avviso shutdown all'utente */
    _createExitMessage(nextMsgToSnd);
    _sendMessageToWriter_(prvsStatus->outMsg, ;);

  } else { /* la terminazione di tale thread e' dovuta ad un errore o
	      ad una richiesta dell'utente connesso */

    /* richiedi la terminazione del thread mittente, cio' e'
       necessario nel caso in cui la terminazione del thread ricevente
       sia staa causata dalla ricezione del messaggio MSG_EXIT, in tal
       caso viene inviato al thread mittente il messaggio MSG_OK */
    nextMsgToSnd.type = MSG_exit;
    nextMsgToSnd.length = 0;
    nextMsgToSnd.buffer = NULL;
    _sendMessageToWriter_(prvsStatus->outMsg, ;);
  }
    
  /* rimuovi il nome utente dalla lista degli utenti connessi */
  if ( -1 == blockingList_remove(conntedUsrList, usrName) )
    perror("msgserv-rcver: blockingList_remove");

  free(prvsStatus);
  free(usrName);

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

  /* nota: la chiusura della connessione e' affidata al
           workerSenderThread che deve inviare l'ultimo messaggio al
           client (MSG_EXIT se la terminazione e' dovuta al
           verificarsi di un errore interno o se l'intero processo e'
           terminato, oppure MSG_OK se la terminazione e' stata
           richiesta dall'utente */
}

#define _workerThread_close						\
									\
  if ( nextMsgToSnd.buffer ) {                                       	\
    Free(nextMsgToSnd.buffer);		                                \
  }                                                                     \
                                                                        \
  if ( nextMsgToRcv.buffer ) {         					\
    Free(nextMsgToRcv.buffer);                             		\
  }                                                                     \
                                                                        \
  if ( -1 == pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) )	\
    perror("msgserv-rcver: pthread_setcancelstate");


void *
workerReceiverTask(void * arg)
{
  int sock_fd = (int) arg;	/* f.d. del socket associato al
				   thread corrente */

  int localLeave = 0;		/* 0 non terminazione
				 * 1 terminazione richiesta dal client
				 * 2 terminazione dovuta ad un errore 
				 */
  /* il nome dell'utente a cui il thread corrente e' associato (in
     lettura) noto dopo il login ell' utente */
  char * myUsrName = NULL;
  int myUsrName_length = 0;

  sigset_t mySigmask_ss;

  /* variabili usate per creare messaggi in ingresso e in uscita */
  message_t nextMsgToRcv, nextMsgToSnd;

  int bufferLength = 0, err = 0;

  /* struttura per impostare lo stato dell'utente dopo il login, e il
     riferimento allo stato precedente */
  userStatus_t userStatus, * prvsUserStatus = NULL, offline;

  pthread_t thisThr = pthread_self();

  void * sender_arg[3];
  
  if (dbg) fprintf(stderr, "---DBG--- msgserv-rcver: start\n"); /* REMOVE ME */

  initOfflineUser(offline);

  /*
   *   FILTRO TUTTI I SEGNALI
   */
  sigfillset(&mySigmask_ss);
  if ( 0 != pthread_sigmask(SIG_SETMASK, &mySigmask_ss, NULL) ) {
    perror("workerTask: pthread_sigmask");
    return NULL;
  }       

  /* 
   *   ATTENDI IL MESSAGGIO DI LOGIN
   */
  if ( 1 > (myUsrName_length = receiveMessage(sock_fd, &nextMsgToRcv)) ) { /* BLOCKING */

      if (0 == myUsrName_length) {
	/* nome utente vuoto*/
	_sendErrorMessage_(_comerr_userNameEmpty_string);
      } else if (-1 == myUsrName_length) {
	/* errore in lettura */
	perror("workerTask: receiveMessage-login");
	_sendErrorMessage_(_comerr_closeConnection_string);
      } else if (SEOF == bufferLength) {
	/* il peer si e' disconnesso */
	; /* do nothing */
      }

      close(sock_fd);
      return NULL;
  }

  if (nextMsgToRcv.type != MSG_CONNECT) {

    /* il tipo del primo messaggio deve essere di login */
    _sendErrorMessage_(_comerr_msgTypeNotConn_string);
    free(nextMsgToRcv.buffer);
    close(sock_fd);
    return NULL;
  }

  myUsrName = nextMsgToRcv.buffer;

  pthread_testcancel();
  

  
  if (2==dbg) fprintf(stderr, "---DBG--- msgserv-rcver: username = %s\n", myUsrName);


  /*
   *   VERIFICA SE L'UTENTE E' AUTORIZZATO 
   *
   *   nel caso: 
   *   - crea una lista dei messaggi in uscita per la connessione
   *     dell'utente,
   *   - associa tale lista all'utente nella usrTable
   *   - crea ed avvia il thread mittente che gestisce l'invio dei
   *     messaggi per tale utente, 
   *     nota: il messaggio di conferma login e' inviato 
   *           automaticamente all'avvio del thread mittente
   *
   * nota: durante la ricerca dell'utente nella userTable e
   *       (l'eventuale) aggiornamento della userTable si deve avere
   *       accesso esclusivo alla userTable 
   *
   */

  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); /* <<<< NO CANCELLAZIONE */

  userTable_lock(); /* BLOCKING */

  if ( NULL == (prvsUserStatus = userTable_find(usrTable, myUsrName)) ) {
    /* l'utente NON E' AUTORIZZATO oppure si e' verificato un errore
       in find_hashElement */
    free(nextMsgToRcv.buffer);
    close(sock_fd);
    userTable_unlock();
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    return NULL;
  }
  if ( USRSTAT_CONNECTED == prvsUserStatus->status ) {
    /* l'utente E' GIA' CONNESSO */
    _sendErrorMessage_(_comerr_userAlreadyConn_string);
    close(sock_fd);
    free(prvsUserStatus);
    free(myUsrName);
    userTable_unlock();
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    return NULL;
  }
  free(prvsUserStatus);

  /* aggiona lo stato dell'utente a CONNESSO e ... */
  userStatus.status = USRSTAT_CONNECTED;
  /* ... crea la LISTA DEI MESSAGGI IN USCITA per l'utente  */
  if ( NULL == (userStatus.outMsg = 
		blockingList_new(0, message_copy, message_compare)) ) {
    perror("workerReceiverTask: blockingList_new-userStatus");
    free(nextMsgToRcv.buffer);
    close(sock_fd);
    userTable_unlock();
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    return NULL;
  }
  
  /* aggiorna la tabella */
  if ( 0 != (err = userTable_set(usrTable, myUsrName, &userStatus, NULL)) ) {
    if (-1 == err) {
      /* errore in setValue_hashTable */
      perror("workerTask-setUserConn");
      _sendErrorMessage_(_comerr_closeConnection_string);
    } else if (-2 == err) {
      /* l'utente non e' autorizzato */
      _sendErrorMessage_(_comerr_userNotAutor_string);
    }

    free(nextMsgToRcv.buffer);
    close(sock_fd);
    userTable_unlock();
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    return NULL;
  }

  /* rilascia l'accesso esclusivo alla tabella degli utenti */
  userTable_unlock();

  pthread_testcancel();

  /* imposta gli argomenti da passare al thread mittente verso
   * l'utente connesso */

  /* la lista dei messaggi di uscita verso l'utente.  Nota: tale
     riferimento e' quello presente nella tabella utenti in quanto la
     funzione userStatus_copy usata all'inizializzazione della tabella
     esegue la copia del riferimento della struttura e non del
     valore */
  sender_arg[0] = userStatus.outMsg; 

  /* riferimento a questo thread, in caso di terminazione il thread
     mittente invia la cancellazione */
  sender_arg[1] = &thisThr;

  sender_arg[2] = (void *) sock_fd;
  
  if ( -1 == threadPool_submit(tpool, workerSenderTask, sender_arg, NULL, NULL) ) {
    if (errno) perror("workerReceiverTask: threadPool_submit");
    _sendErrorMessage_(_comerr_closeConnection_string);
    free(nextMsgToRcv.buffer);
    close(sock_fd);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    return NULL;
  }

  /*
   *   L'UTENTE E' AUTORIZZATO, AGGIUNGO ALLA LISTA DEGLI UTENTI
   *   CONNESSI (l'invio del messaggio di conferma w' inviato dal
   *   thread sender)
   */
  errno = 0;
  if ( -1 == blockingList_push(conntedUsrList, myUsrName) ) { /* BLOCKING */

    perror("workerTask-blockingList_push");
    _sendErrorMessageToWriter_(_comerr_closeConnection_string);
    close(sock_fd);
    userTable_set(usrTable, myUsrName, &offline, NULL);
    free(myUsrName);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    return NULL;
  }

  /*
   *   GESTIONE DELLA CANCELLAZIONE
   */

  pthread_cleanup_push(workerReceiverThread_cleanup, (void *) myUsrName);



  if (dbg) fprintf(stderr, "---DBG--- msgserv-rcver: utente %s connesso sul "
		   "socket %d\n", myUsrName, sock_fd); 

  /*
   *   SERVI IL CLIENTE
   */
  while (!localLeave  /* variabile di terminazione locale al thread */
	 &&  !leave)  /* variabile di terminazione globale al processo msgserv */
  {
    nextMsgToRcv.buffer = NULL; 
    nextMsgToSnd.buffer = NULL;
                                                      /* <<< SI CANCELLAZIONE */
    if ( -1 == pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) ) 
      perror("workerTask: pthread_setcancelstate");

    /*
     *   LEGGI IL PROSSIMO MESSAGGIO DAL SOCKET 
     */
    errno = 0;
    if ( 0 > (bufferLength = 
	      receiveMessage(sock_fd, &nextMsgToRcv)) ) { /* BLOCKING */

      localLeave = 2;

      if (-1 == bufferLength) {
	/* errore in receiveMessage */
	perror("workerTask: receiveMessage-nextMsg");
      } else if (SEOF == bufferLength) {
	/* il peer si e' disconnesso */
	localLeave = 1;
      }

      if (nextMsgToRcv.buffer) {
	Free(nextMsgToRcv.buffer);
      }

      continue;
    }

    if (dbg) fprintf(stderr, "---DBG--- msgserv-rcver: RECEIVED: type = %c, "
		     "buffer = >%s<\n", nextMsgToRcv.type, nextMsgToRcv.buffer);

    /*
     * ESEGUI IL SERVIZIO RICHIESTO                     <<< NO CANCELLAZIONE 
     */
    if ( -1 == pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL) ) 
      perror("workerTask: pthread_setcancelstate");


    switch (nextMsgToRcv.type) {
    case MSG_CONNECT: 
    {
      _createErrorMessage(nextMsgToSnd, _comerr_userAlreadyConn_string);
      _sendMessageToWriter_(userStatus.outMsg, continue);
      break;
     }

     case MSG_EXIT: 
     {
       if (dbg) fprintf(stderr, "---DBG--- msgserv-rcver: utente %s uscito\n", myUsrName);
       
       /* invia la conferma di uscita */
       _createOkMessage(nextMsgToSnd);
       _sendMessageToWriter_(userStatus.outMsg, ;);

       /* il messaggio MSG_exit per far terminare il thread mittente
	  e' inviato nella funzione di cleanup sempre eseguita alla
	  terminazione. cio' assicura che tale messaggio sia
	  sempre(eccetto leave == 1) inviato al thread mittente anche
	  in caso di terminazione dovuta ad un errore di tale
	  thread */

       if (nextMsgToRcv.buffer) { Free(nextMsgToRcv.buffer); }
       localLeave = 1;
       break;
     }

     case MSG_LIST: 
     { 
       /* iteratore della lista degli utenti connessi */
       blockingList_itr_t listItr;
       /* stinga che rappresenta la lista degli utenti connessi */ 
       char * users = NULL;
       /* variaile temporanea usata nell'iterazione della lista di utenti */
       char * nextUser = NULL;
       /* dimensione attuale della stringa users */
       int users_actlSz = 1;

       /* itera tutta la lista degli utenti connessi */
       blockingList_itr_init(&listItr, conntedUsrList);
       blockingList_lock(conntedUsrList);
       while ( NULL != (nextUser = blockingList_itr_getValue(&listItr)) ) 
       {
	 int strlength = strlen(nextUser);
	 void * tmp = NULL;
	 users_actlSz +=  strlength + 1;

	 /* rialloca la memoria per contenere il prossimo nome utente
	    ed il carattere ' ' */
	 if ( NULL == (tmp = realloc(users, users_actlSz)) ) {
	   perror("workerTask: realloc");
	   free(users);
	   localLeave = 2;
	   continue;
	 }
	 users = tmp;

	 /* se e' il primo utente ... */
	 if (strlength == users_actlSz - 2) strcpy(users, nextUser);
	 else  strcat(users, nextUser);

	 if ( blockingList_itr_hasNext(&listItr) )
	   strcat(users, " ");
	 else {
	   /* se e' l'ultimo utente dealloca 1 byte per il carattere 
	      ' '. nota che users_actlsz e' iniz a 1 e che il carattere
	      '\0' e' aggiunto dalle funzioni strcpy e strcat. */
	   if ( NULL == (tmp = realloc(users, users_actlSz-1)) ) {
	     perror("workerTask: realloc");
	     _sendErrorMessageToWriter_(_comerr_closeConnection_string);
	     Free(users);
	     localLeave = 2;
	     continue;
	   }
	 }

	 blockingList_itr_next(&listItr);
       }
       blockingList_unlock(conntedUsrList);

       if (2==dbg) fprintf(stderr, "---DBG--- msgserv-sorker: users = >%s<, actualSize = %d\n",
			users, users_actlSz);

       /* crea il messaggio ed invialo al thread mittente */
       _createListMessage_(nextMsgToSnd, users, users_actlSz - 1);
       _sendMessageToWriter_(userStatus.outMsg, localLeave = 2; continue);
       
       break;
     }

     case MSG_TO_ONE:
     {
       char * receiverName = NULL, 
	 * messageString = NULL,    
	 * logMessageString,        /* formato "senderName:receiverName:message" */
	 * toSendMessageString;     /* formato "[senderName] message" */

       userStatus_t * outMesgList_dest = 0;

       if ( 1 > nextMsgToRcv.length ) continue;

       /* nextMsgToRcv.buffer e' nel formato "receiverName\0messageString" */

       receiverName = nextMsgToRcv.buffer;
       messageString = receiverName + strlen(receiverName) + 1;

       if (2==dbg) fprintf(stderr, "---DBG--- msgserv-rcver: msg.length = %d, "
			"msg.buffer = >%s...<, receiver = >%s<, message = >%s<\n",
			nextMsgToRcv.length, nextMsgToRcv.buffer, 
			receiverName, messageString);

       if ( '\0' == *messageString ) {
	 /* il messaggio e' vuoto, non fare nulla */
	 continue;
       }

       userTable_lock();

       errno = 0;
       if ( NULL == (outMesgList_dest = 
		     userTable_find(usrTable, receiverName)) ) {

	 if (errno) {
	   /* errore in findUser */
	   perror("workerTask: findUser-to_one");
	   localLeave = 2;
	 } else {
	   /* il nome utente non e' nella tabella degli utenti
	    * autorizzati, INVIA UN MESSAGGIO DI ERRORE */
	   char * errMsg = NULL;

	   userTable_unlock();

	   if ( NULL == (errMsg = malloc(strlen(receiverName) + 
					 strlen(_comerr_oneRcverNotAuth_string) +
					 3)) ) {
	     _sendErrorMessageToWriter_(_comerr_closeConnection_string);
			       
	     localLeave = 2;
	     continue;
	   }
	   strcpy(errMsg, receiverName);
	   strcat(errMsg, ": ");
	   strcat(errMsg, _comerr_oneRcverNotAuth_string);

	   _sendErrorMessageToWriter_(errMsg); 
	   free(errMsg);
	 }

	 continue;
       }

       userTable_unlock()

       if ( USRSTAT_OFFLINE == outMesgList_dest->status ) {
	 /* il destinatario richiesto non e' connesso, INVIA UN
	  * MESSAGGIO DI ERRORE */
	 char * errMsg = NULL;
	 if ( NULL == (errMsg = malloc(strlen(receiverName) + 
				       strlen(_comerr_oneRcverNotConn_string) +
				       3)) ) {
	   
	   _sendErrorMessageToWriter_(_comerr_closeConnection_string);

	   Free(outMesgList_dest);
	   localLeave = 2;
	   continue;
	 }
	 strcpy(errMsg, receiverName);
	 strcat(errMsg, ": ");
	 strcat(errMsg, _comerr_oneRcverNotConn_string);
	 
	 _sendErrorMessageToWriter_(errMsg);

	 free(errMsg);
	 Free(outMesgList_dest);
	 continue;
       }

       /* crea il messaggio da spedire al singolo utente nel formato
	* "[senderName] messageString"
	*/
       if ( NULL == (toSendMessageString = 
		     createMsgString(myUsrName, messageString, myUsrName_length)) ) {
	 /* errore non gestibile, invia messaggio di errore e chiudi
	    la connessione */
	 if (errno) perror("workerTask: createMsgString-to_one");
	 else fprintf(stderr, "workerTask: createMsgString-to_one: error\n");
	 _sendErrorMessageToWriter_(_comerr_closeConnection_string);

	 localLeave = 2;
	 continue;
       }

       if (dbg) fprintf(stderr, "---DBG--- msgserv-rcver: message_str = >%s<\n",
			   toSendMessageString);

       /* invia il messaggio al thread scrittore per l'invio all'utente */
       _createToOneMessage(nextMsgToSnd, toSendMessageString);
       _sendMessageToWriter_(outMesgList_dest->outMsg, ;);

       /* crea ed invia il messaggio di log al log-writer thread
	* "senderName:receiverName:messageString"
	*/
       _sendLogMsg(logMessageString, receiverName, messageString, "one");

       Free(logMessageString);
       Free(outMesgList_dest);
	 
       break;
     } 

     case MSG_BCAST:
     {
       blockingList_itr_t listItr;
       char * nextUser = NULL;
       char * nextLogString = NULL;
       char * nextMsgString = NULL;
       userStatus_t *  nextDest = 0;

       /* crea la stringa del messaggio da inviare in broadcast 
	* "[senderName] messageString"
	* ed il messaggio stesso.
	*/
       if ( NULL == (nextMsgString = createMsgString(myUsrName, 
						     nextMsgToRcv.buffer,
						     myUsrName_length)) ) {
	 if (errno) perror("msgserv: workerTask: createMsgString");
	 localLeave = 2;
	 continue;
       }
       _createBcastMesssage(nextMsgToSnd, nextMsgString);


       if (2==dbg) fprintf(stderr, "---DBG--- msgserv-rcver: bcast: messaggio = >%s<\n",
			   nextMsgString);

       /* itera tutta la LISTA DEGLI UTENTI CONNNESSI, per ogni
	  utente invia la stringa creata precedentemente */
       blockingList_itr_init(&listItr, conntedUsrList);
       blockingList_lock(conntedUsrList);
       while ( NULL != (nextUser = blockingList_itr_getValue(&listItr)) )
       {
	 userTable_lock();

	 /* trova la coda dei messaggi in uscita dell'utente attuale */
	 if ( NULL == (nextDest = userTable_find(usrTable, nextUser)) ) {
	   /* errore non gestibile, invia messaggio di errore e chiudi
	      la connessione */
	   if (errno) {
	     perror("workerTask:findUser-bcast");
	     
	     userTable_unlock();

	     _createErrorMessage(nextMsgToSnd, _comerr_closeConnection_string);
	     _sendMessageToWriter_(userStatus.outMsg, ;);

	     Free(nextMsgString);
	     localLeave = 2;
	     continue;
	   } else {
	     ; /* autente non autorizzato, non dovrebbe mai succedere! */
	   }
	 }

	 userTable_unlock();
	 
	 /* invia il messaggio alla coda dei messaggi in uscita del
	    prossimo utente connesso */
	 _sendMessageToWriter_(nextDest->outMsg, ;);

	 /* crea la stringa del messaggo di log e invialo al thread
	  * logwriter 
	  * "senderName:nextUserName:messageString"
	  */
	 _sendLogMsg(nextLogString, nextUser, nextMsgToRcv.buffer, "bcast");

	 /* libera la memoria occupata in questa iterazione */
	 Free(nextLogString);
	 Free(nextDest);

	 blockingList_itr_next(&listItr);
       }
       blockingList_unlock(conntedUsrList);

       break;
     }
     default: 
       _createErrorMessage(nextMsgToSnd, _comerr_wrongMsgType_string);
       _sendMessageToWriter_(userStatus.outMsg, ;);

    } /* switch (nextMsgToRcv.type) */


    if ( nextMsgToRcv.buffer ) { Free(nextMsgToRcv.buffer); }
    if ( nextMsgToSnd.buffer ) { Free(nextMsgToSnd.buffer); }


    if (dbg) fprintf(stderr, "---DBG--- msgserv-rcver: SERVED\n");

   } /* while (!localLeave  &&  !leave) */

   if (dbg) fprintf(stderr, "---DBG--- msgserv-rcver: closing\n"); /* REMOVE ME */


   if ( nextMsgToRcv.buffer ) { Free(nextMsgToRcv.buffer); }
   if ( nextMsgToSnd.buffer ) { Free(nextMsgToSnd.buffer); }

   if ( 2 == localLeave  &&  !leave) { /* se NON e' arrivato un
				 	  segnale E la terminazione e'
				 	  dovuta ad un errore non
				 	  gestibile */
     _createExitMessage(nextMsgToSnd);
     _sendMessageToWriter_(userStatus.outMsg, ;);
   }

   pthread_cleanup_pop(1); /* esegue le operazioni di chiusura */

   if (dbg) fprintf(stderr, "---DBG--- msgserv-rcver: end\n"); /* REMOVE ME */

   return (void *) 0;
 }
