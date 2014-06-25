/**
 *  \file msgservWorkerSender.c
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
#include "comsock.h"
#include "blockingList.h"

extern char dbg;
extern volatile sig_atomic_t leave;



#define _sendMessage(socket, msgVar, retVar, errTodo, intrTodo)		\
  if ( 0 > (retVar = sendMessage(sock_fd, &msgVar)) ) {			\
									\
    if ( -1 == retVar ) {						\
      if (EINTR == errno) intrTodo;					\
      if ( errno ) perror("sendMessage");                               \
      else fprintf(stderr, "sendMessage: error\n");                     \
    } else if ( SEOF == retVar ) {					\
      ; /* do nothing */                                                \
    }									\
									\
    errTodo;								\
  }									

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
  _sendErrorMessage(sock_fd, nextOutMsg, _errStr, ;)



void
workerSenderThread_cleanup(void * arg)
{
  ;
}


void *
workerSenderTask(void * arg)
{
  blockingList_t * outMsgList = (blockingList_t *) ((void **) arg) [0];
  pthread_t *      rcverThr =        (pthread_t *) ((void **) arg) [1];
  int              sock_fd =                 (int) ((void **) arg) [2];

  message_t * nextOutMsg;
  message_t okmsg;
  sigset_t mySigmask_ss;
  
  /* valori di localLeave:
   * 0  non terminazione
   * 1  terminazione richiesta dal receiver thread o dall'utente
   * 2  terminazione dovuta al verificarsi di un errore di tale task
   */
  int localLeave = 0, err = 0;

  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);


  /* filtro tutti i segnali */
  sigfillset(&mySigmask_ss);
  if ( 0 != pthread_sigmask(SIG_SETMASK, &mySigmask_ss, NULL) ) {
    perror("workerTask: pthread_sigmask");
    return NULL;
  }       

  /* invio messaggio di conferma alla richiesta di login */
  _createOkMessage(okmsg);
  if ( 0 != (err=sendMessage(sock_fd, &okmsg)) ) {
    if ( -1 == err ) {
      perror("workerTask-sendMessage");
      _sendErrorMessage(sock_fd, okmsg, _comerr_closeConnection_string, ;);
    } else if ( SEOF == err ) {
	;
    }
    localLeave = 2;
  }

  while ( !localLeave ) 
  {
    /* estrai il prossimo messaggio in usicta (lista FIFO) */
    if ( -1 == takeMessage(outMsgList, &nextOutMsg) ) {
      if (errno) perror("workerSenderTask: popMessage");
      localLeave = 2;
      continue;
    }

    if (dbg) fprintf(stderr, "---DBG--- msgserv-snder: TO SEND: type = %c, "
		     "length = %d, buffer = >%s<, socket = %d\n",
		     nextOutMsg->type, nextOutMsg->length,
		     nextOutMsg->buffer, sock_fd);


    /* verifica correttezza messaggio */
    if (0 < nextOutMsg->length  &&  NULL == nextOutMsg->buffer ) 
      continue; /* ignora messaggio */


    /* controlla che non sia un messaggio di chiusura all'utente
       oppure a me stesso */
    if ( MSG_exit == nextOutMsg->type ) { 
      localLeave = 1;
      continue;
    } else if ( MSG_EXIT == nextOutMsg->type ) 
      localLeave = 1;


    /* invia il messaggio al cliente */
    if ( 0 != (err = sendMessage(sock_fd, nextOutMsg)) ) {
      if ( -1 == err) {
	if (errno) perror("workerSenderTask: sendMessage");
      } else if ( SEOF == err ) ;
      localLeave = 2;
      continue;
    }

    /* libera la memoria occupata dal messaggio */
    if (NULL != nextOutMsg->buffer) { 
      Free(nextOutMsg->buffer); 
    }
    Free(nextOutMsg); 

  } /* while ( !localLeave) */

  if (dbg) fprintf(stderr, "msgserv-snder: closing, thr = %d\n", 
		   (int) pthread_self());

  if ( localLeave ) {

    if (dbg) fprintf(stderr, "---DBG--- msgserv-snder: term: free\n");
    if (nextOutMsg  &&  nextOutMsg->buffer) 
      free(nextOutMsg->buffer);
    free(nextOutMsg);

    /* se si e' verificato un errore interno a tale task cancella il
       thread ricevente altrimenti la terminazione e' stata richiesta
       dal thread ricevente */
    if (2 == localLeave ) {
      if (dbg) fprintf(stderr, "---DBG--- msgserv-snder: term: cancel rcver\n");
      pthread_cancel(*rcverThr); 
    }

  }

  blockingList_free(&outMsgList);
  close(sock_fd);


  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);


  if (dbg) fprintf(stderr, "msgserv-snder: term, thr = %d\n", 
		   (int) pthread_self());

  return NULL;
}
