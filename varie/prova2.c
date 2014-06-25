#include <time.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

#include "errorUtil.h"
#include "comsock.h"
#include "genHash.h"
#include "threadPool.h"
#include "blockingList.h"
#include "blockingList_itr.h"


int compare_int(void *a, void *b) {
    int *_a, *_b;
    _a = (int *) a;
    _b = (int *) b;
    return ((*_a) - (*_b));
}
int compare_string(void *a, void *b) {
    char *_a, *_b;
    _a = (char *) a;
    _b = (char *) b;
    return strcmp(_a,_b);
}
/* funzione di copia di un intero */
void * copy_int(void *a) {
  int * _a;

  if ( ( _a = malloc(sizeof(int) ) ) == NULL ) return NULL;

  *_a = * (int * ) a;

  return (void *) _a;
}
/* funzione di copia di una stringa */
void * copy_string(void * a) {
  char * _a;

  if ( ( _a = strdup(( char * ) a ) ) == NULL ) return NULL;

  return (void *) _a;
}



define_hashTable_addElement(addUser, char *, int *)
define_hashTable_findElement(findUser, char *, int *)
define_hashTable_setValue(setUserConn, char *, int *)

define_blockingList_push(pushLogMessage, char *)
define_blockingList_pop(popLogMessage, char **)




/*
 *   STRUTTURE DATI CONDIVISE TRA I THREAD DEL PROCESSO 
 */

blockingList_t * logStack = NULL; /* deallocata da writer-thread */


#define _usrOffline -1

void *
workerTask(void * arg)
{
  ;
  return (void *) 0;
}


/*
 *   TASK e FUNZIONE DI CLEAN UP DEL WRITER THREAD
 */

void
writerThread_cleanup(void * arg)
{
  char * nextLog = NULL;

  printf("---DBG--- SERV-WRITER: CLEAN UP\n");

  blockingList_unlock(logStack); 

  while (!blockingList_isEmpty(logStack)) {

    popLogMessage(logStack, &nextLog);

  }

  blockingList_free(&logStack);

  printf("---DBG--- SRV-WRITER: CLEAN UP: TERM\n");
}

void *
writerTask(void * arg)
{
  sigset_t mySigmask_ss;
  char * nextLog = NULL;
  char ** nextLog_ = NULL;
  
  /*
   *   FILTRO TUTTI I SEGNALI 
   */

  sigfillset(&mySigmask_ss);

  _handle_ptherr_exit(  pthread_sigmask(SIG_SETMASK, 
					&mySigmask_ss, NULL),
			"writer", "pthread_sigmask"  );

  pthread_cleanup_push(writerThread_cleanup, NULL);

  nextLog_ = malloc(sizeof(char *));
  /* printf("nextLog_ = %p,  *nextLog_ = %p \n", nextLog_, *nextLog_); */

  printf("---DBG--- SERV-WRITER: START\n");
  while (1)
  {
    /*popLogMessage(logStack, &nextLog);*/ /* C.P. */
    /*popLogMessage(logStack, nextLog_);*/ /* C.P. */
    blockingList_pop(logStack, (void **)&nextLog);

    printf("%s\n", nextLog); /* REMOVE ME */

  }


  pthread_cleanup_pop(0);

  blockingList_free(&logStack);

  printf("---DBG--- SERV-WRITER: TERMINATED\n");

  return (void *) 0;
}



void
signalHandler_exit(int sig)
{
  write(1, "Ricevuto un segnale, esco.\n", 27);
  ;				/* TODO */
}


void
signalHandler_ignoreIt(int sig)
{
  write(1, "Ricevuto un segnale, ignorato.\n", 31);
  ; 				/* DO NOTHING */
}


#define _maxLengthUserName 256
#define _maxLengthUserName_string "256"
#define _sizeHashTable 100
#define _threadPool_minSize 10
#define _threadPool_maxSize 100
#define _lsock_path "./tmp/msgsock"


void
mainThread_cleanup(void *arg)
{
  int * lsock =                     (int *) ((void **) arg) [0];
  threadPool_t ** tpool = (threadPool_t **) ((void **) arg) [1];
  pthread_t * writerThr =     (pthread_t *) ((void **) arg) [2];

  printf("---DBG--- MAIN: ERR. CLOSING\n");

  /* chiudi il socket di ascolto */

  if ( -1 != *lsock ) {
    unlink(_lsock_path);
    closeSocket(*lsock);
  }

  /* cancella tutti i thread lavoratori ed attendi la loro
     teminazione */

  /* cancella il thread di log ed attendi la terminazione */

  pthread_cancel(*writerThr);
  pthread_join(*writerThr, NULL);

  printf("---DBG--- MAIN: ERR. CLOSING: TERMINATED\n");
}

int
main(int argc, char ** argv)
{
  /* mappe di segnali */
  sigset_t allSignal_ss, oldSignalConfig_ss;
  
  /* azioni su segnali */
  struct sigaction ignore_sa, exit_sa;
  
  /* socket usati */
  int lsock = -1;	/* socket di ascolto */
  int asock = -1;      	/* variabile temporanea per un socket attivo */

  int numOfItemRead = 0, i = 0, leave = 0;

  threadPool_t * tpool = NULL;	
  pthread_t writerThr;

  void * cleanUp_arg[3];

  bzero(&writerThr, sizeof(pthread_t));
  cleanUp_arg[0] = (void *) &lsock;
  cleanUp_arg[1] = (void *) &tpool;
  cleanUp_arg[2] = (void *) &writerThr;
  pthread_cleanup_push(mainThread_cleanup, cleanUp_arg);

  /*
   *   MODIFICA SIGNAL HANDLER ARRAY DEL PROCESSO
   */

  sigfillset(&allSignal_ss);

  bzero(&ignore_sa, sizeof(ignore_sa));
  ignore_sa.sa_handler = signalHandler_ignoreIt;
  ignore_sa.sa_mask = allSignal_ss;

  bzero(&exit_sa, sizeof(exit_sa));
  exit_sa.sa_handler = signalHandler_exit;
  exit_sa.sa_mask = allSignal_ss;

  /* filtro temporaneamente tutti i segnali per settare la s.h.a. */

  _handle_ptherr_exit(  pthread_sigmask(SIG_SETMASK, &allSignal_ss, 
					&oldSignalConfig_ss),
			"main", "pthread_sigmask-0"  );
  /*
  _handle_meno1err_exit(  sigaction(SIGINT, &exit_sa, NULL),
  "main", "sigaction-int"  );*/
  _handle_meno1err_exit(  sigaction(SIGTERM, &exit_sa, NULL),
			  "main", "sigaction-term"  );
  _handle_meno1err_exit(  sigaction(SIGPIPE, &ignore_sa, NULL),
			  "main", "sigaction-pipe"  );
  _handle_meno1err_exit(  sigaction(SIGUSR1, &ignore_sa, NULL),
			  "main", "sigaction-usr1"  );

  /* ripristiono la signal mask */
  
  _handle_ptherr_exit(  pthread_sigmask( SIG_SETMASK, 
					 &oldSignalConfig_ss, NULL), 
			"main", "pthread_sigmask-1"  );

  /* 
   *   CREA LE ALTRE STRUTTURE DATI CONDIVISE TRA I THREAD
   *   log-file, log-stack, conntedUsr-list
   */

  _handle_nullerr_exit(  logStack = 
			 blockingList_new(0, copy_string, compare_string),
			 "main", "blockingList_new-logStack"  );
     
  /*
   *   AVVIA IL thread-pool ED IL writer-thread
   */

  _handle_ptherr_exit(  pthread_create(&writerThr, NULL, writerTask, NULL),
			"main", "pthread_create-writer"  );
  
  /* 
   *   CREA IL SOCKET DI ASCOLTO
   */

  _handle_meno1err_ptexit_str(  lsock = 
				createServerChannel(_lsock_path),
				"main", "createServerChannel",
				_lsock_path );

  while (!leave)
  {
    /*
     *   RIMANE IN ATTESA DI RICHESTE DI CONNESSIONE SU l-sock 
     */

    printf("MAIN: ACCEPT\n");
    errno = 0; 
    if ( -1 ==  ( asock = acceptConnection(lsock) ) ) {
      
      if ( EINTR == errno ) {
	/* e' richiesta la terminazioe del processo */
	leave = 1;
	continue;
      } else {
	perror("main: acceptConnection");
	pthread_exit((void *) errno);
      }

    }

    /*
     *   CREA UN NUOVO worker-thread PER LA CONNESSIONE STABILITA
     */


  }

  /*
   *   TERMINAZIONE GRADUALE DI TUTTI I THREAD LAVORATORI
   */

  printf("---DBG--- MAIN: CLOSING\n");

  _handle_meno1err_ptexit(  unlink(_lsock_path),
			    "main", "unlink"  );
  _handle_meno1err_ptexit(  closeSocket(lsock),
			    "main", "closeSocket"  );


  pthread_cancel(writerThr);
  _handle_ptherr_exit(  pthread_join(writerThr, NULL),
			"main", "pthread_join"  );

  pthread_cleanup_pop(0);

  {
    struct timespec tosleep;

    tosleep.tv_nsec = 0;
    tosleep.tv_sec = 1;
    nanosleep(&tosleep, NULL);
  }

  printf("---DBG--- MAIN: TERMINATED\n");

  return 0;
}

