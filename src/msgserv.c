/**
   \file msgserv.c
   \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
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

#include "msgserv.h"
#include "errorUtil.h"
#include "comsock.h"
#include "genHash.h"
#include "threadPool.h"
#include "blockingList.h"
#include "blockingList_itr.h"


#ifdef dbg_msgserv 
  char dbg = 1;
#elif defined dbg_msgserv_
  char dbg = 2;
#else
  char dbg = 0;
#endif

#ifdef _notest
  static char notest = 1;
#else
  static char notest = 0;
#endif



#define _exename "msgserv"

#define forceQuit_incorrectUserList 1


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

void * 
userStatus_copy(void * a)
{
  userStatus_t * _a;
  
  if ( NULL == (_a = malloc(sizeof(userStatus_t))) ) 
    return NULL;

  _a->status = ((userStatus_t *)a)->status;
  _a->outMsg = ((userStatus_t *)a)->outMsg;

  return (void *) _a;
}
int
userStatus_compare(void * a, void * b)
{
  return (int) a - (int) b;
}


define_hashTable_addElement(userTable_add, char *, userStatus_t *)
define_hashTable_findElement(userTable_find, char *, userStatus_t *)
define_hashTable_setValue(userTable_set, char *, userStatus_t *)

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
int 
userTable_mutex(int dolock)
{
  static pthread_mutex_t usrTable_mtx;
  static pthread_t owner;
  static int locked = 0;
  static int init = 0;

  if ( !init ) {
    init = 1;
    if( 0 != pthread_mutex_init(&usrTable_mtx, NULL) )
      return -1;
    bzero(&owner, sizeof(pthread_t));
  }

  if ( 0 < dolock ) { /* blocca */
    if ( locked  && pthread_self() == owner ) {
      errno = EDEADLK; return -1;
    }
    if ( 0 != pthread_mutex_lock(&usrTable_mtx) ) return -1;
    locked = 1;
    owner = pthread_self();

  } else if ( !dolock ) { /* sblocca */
    if ( !locked) return 0;
    if ( owner != pthread_self() ) { errno = EPERM; return -1; }
    if ( 0 != pthread_mutex_unlock(&usrTable_mtx) ) return -1;
    locked = 0;

  } else { /* distruggi */
    if ( !locked ) locked = 1;
    else if ( locked  &&  owner != pthread_self() ) {
      errno = EPERM; return -1;
    }
    if ( 0 != pthread_mutex_destroy(&usrTable_mtx) ) return -1;
    init = 0;

  }

  return 0;
}


define_blockingList_push(pushLogMessage, char *)
define_blockingList_pop(popLogMessage, char **)

define_blockingList_add(putMessage, message_t *)
define_blockingList_poll(takeMessage, message_t **)



/*
 *   STRUTTURE DATI CONDIVISE TRA I THREAD DEL PROCESSO 
 */

FILE * logFile;			  /* chiuso da wirter-thread */
hashTable_t * usrTable = NULL;	  /* (de)allocata da main-thread */
blockingList_t * logStack = NULL; /* deallocata da writer-thread */
blockingList_t * conntedUsrList = NULL;  /* (de)allocata da main-thread */
threadPool_t * tpool = NULL;	



/* variabile intera per stabilere la terminazione del processo. e'
   scritta dal gestore dei segnali di uscita */
volatile sig_atomic_t leave = 0;

void
signalHandler_exit(int sig)
{
  write(2, "Ricevuto un segnale, esco.\n", 27); /* REMOVE ME */
  leave = 1;
}

void
signalHandler_ignoreIt(int sig)
{
  write(2, "Ricevuto un segnale, ignorato.\n", 31); /* REMOVE ME */
  ; 				/* DO NOTHING */
}

#define _maxLengthUserName 256
#define _maxLengthUserName_string "256"
#define _sizeHashTable 100
#define _threadPool_minSize 10
#define _threadPool_maxSize 100
#define _lsock_path "/tmp/msgsock"

void
mainThread_cleanup(void *arg)
{
  int * lsock =                     (int *) ((void **) arg) [0];
  threadPool_t ** tpool = (threadPool_t **) ((void **) arg) [1];
  pthread_t * writerThr =     (pthread_t *) ((void **) arg) [2];

  if (dbg) fprintf(stderr, "---DBG--- msgserv-main: cleanup: start\n");

  /* chiudi il socket di ascolto */

  if ( -1 != *lsock ) {
    unlink(_lsock_path);
    closeSocket(*lsock);
  }

  /* cancella tutti i thread lavoratori ed attendi la loro
     teminazione */

  threadPool_shutdownNow(*tpool);
  threadPool_awaitTermination(*tpool);

  if (dbg) fprintf(stderr, "msgserv-main: cleanup: fine awaitTermination\n");

  /* cancella il thread di log ed attendi la terminazione */

  pthread_cancel(*writerThr);
  pthread_join(*writerThr, NULL);

  /* libera la memoria occupata dalle strutture dati condivise tra i
     thread */ 

  free_hashTable(&usrTable);
  blockingList_free(&conntedUsrList);
  blockingList_free(&logStack);

  if (dbg) fprintf(stderr, "---DBG--- msgserv-main: cleanup: end\n");
}

int
main(int argc, char ** argv)
{
  /* mappe di segnali */
  sigset_t allSignal_ss, oldSignalConfig_ss;
  
  /* azioni su segnali */
  struct sigaction ignore_sa, exit_sa;
  
  char * usrAutrFile_name = NULL, * logFile_name = NULL;
  FILE * usrAutrFile;

  /* variabili temporanee per inizializzare l'hash table */
  char nextUsr[_maxLengthUserName];
  userStatus_t nextUsrState; /* user is offline */

  /* socket usati */
  int lsock = -1;	/* socket di ascolto */
  int asock = -1;      	/* variabile temporanea per un socket attivo */

  int numOfItemRead = 0, i = 0;

  pthread_t writerThr;

  void * cleanUp_arg[3];

  /* inizializza la struttura che rappresente lo stato offline di un
     utente autorizzato (inizialmente lo sono tutti) */
  initOfflineUser(nextUsrState);

  bzero(&writerThr, sizeof(pthread_t));
  cleanUp_arg[0] = (void *) &lsock;
  cleanUp_arg[1] = (void *) &tpool;
  cleanUp_arg[2] = (void *) &writerThr;
  pthread_cleanup_push(mainThread_cleanup, cleanUp_arg);

  /*
   *   VERIFICA ARGOMENTI 
   */

  if (argc != 2+1) {
    if (notest) fprintf(stderr, "Usage: msgserv file_utenti_autorizzati file_log\n");
    return -1;
  }

  usrAutrFile_name = argv[1];
  logFile_name = argv[2];

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

  _handle_meno1err_exit(  sigaction(SIGINT, &exit_sa, NULL),
			  "main", "sigaction-int"  );
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
   *   GENERA HASH TABLE DEGLI UTENTI AUTORIZZATI
   */

  _handle_nullerr_exit(  usrAutrFile = fopen(usrAutrFile_name, "r"),
			 "main", "fopen"  );

  _handle_nullerr_exit(  usrTable = 
			 new_hashTable(_sizeHashTable, compare_string,
				       copy_string, userStatus_copy,
				       hash_string),
			 "main", "new_hashTable"  );
  i=0;
  while ( EOF != ( numOfItemRead =
		   fscanf(usrAutrFile, "%"_maxLengthUserName_string"s\n",
			  nextUsr) ) )
  {
    /* verifca correttezza user name */
    char * itr = nextUsr;

    i++;

    if (0 == numOfItemRead) {
      /* la riga corrente e' vuota o non leggibile */
      if (notest) fprintf(stderr, "%s: impossibile leggere la riga %d.\n", 
			   usrAutrFile_name, i);

      if (forceQuit_incorrectUserList) {
	fclose(usrAutrFile);
	free_hashTable(&usrTable);
	exit(1);
      } else
	continue;
    }

    if (_maxLengthUserName == strlen(nextUsr) 
	&&  '\0' != nextUsr[_maxLengthUserName]) {
      /* la riga corrente supera la dimensione massima di nome utente */
      if (notest) fprintf(stderr, "%s: riga %d ignorata, il numero dei" 
			   " caratteri e' superiore a %s.\n",		
			   usrAutrFile_name, i, _maxLengthUserName_string);
      if (forceQuit_incorrectUserList) {
	fclose(usrAutrFile);
	free_hashTable(&usrTable);
	exit(1);
      } else
	continue;
    }

    /* verifica che tutti i caratteri della riga corrente siano alfanumerici */
    while ('\0' != itr  &&  isalnum(*itr)) itr++;

    if (*itr != '\0') {
      /* la riga corrente non e' alfanumerica */
      if (notest) fprintf(stderr, "%s: riga %d ignorata, i caratteri non sono "
			   "alfanumerici.\n", usrAutrFile_name, i);
      if (forceQuit_incorrectUserList) {
	fclose(usrAutrFile);
	free_hashTable(&usrTable);
	exit(1);
      } else
	continue;
    }	      

    /* aggiungi l'utente alla tabella hash */
    _handle_meno1err_exit(  userTable_add(usrTable, nextUsr, &nextUsrState),
			    "main", "add_hashElement"  );
    /*
    if (dbg) fprintf(stderr, "---DBG--- msgserv: aggiunto a user table: >%s<\n", 
		     nextUsr);
    */
  }

  _handle_meno1err_exit(  fclose(usrAutrFile),
			  "main", "fclose"  );

  /* 
   *   CREA LE ALTRE STRUTTURE DATI CONDIVISE TRA I THREAD
   *   log-file, log-stack, conntedUsr-list
   */

  _handle_nullerr_exit(  logFile = fopen(logFile_name, "w"),
			   "main", "fopen"  );  
   
  _handle_nullerr_exit(  logStack = 
			 blockingList_new(0, copy_string, compare_string),
			 "main", "blockingList_new-logStack"  );
  
  _handle_nullerr_exit(  conntedUsrList = 
			 blockingList_new(0, copy_string, compare_string),
			 "main", "blockingList_new-conntedUsrList");

  /*
   *   AVVIA IL thread-pool ED IL writer-thread
   */

  _handle_nullerr_ptexit(  tpool =
			   threadPool_newCached(60),
			   /* threadPool_newFixed(10, 10), */
			   "main", "threadPool_newCached"  );

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
     * RIMANE IN ATTESA DI RICHESTE DI CONNESSIONE SU l-sock
     */
    if (dbg) fprintf(stderr, "---DBG--- msgserv-main: accept\n");
    errno = 0; 
    if ( -1 ==  ( asock = acceptConnection(lsock) ) ) {
      
      if ( EINTR == errno ) {
	/* potrebbe essere richiesta la terminazioe del processo */
	continue;
      } else {
	perror(_exename": acceptConnection");
	pthread_exit((void *) errno);
      }
    }

    /* crea ed avvia il thread lavoratore ricevente assegnato alla
       connessione appena stabilita, tale thread esegue il login del
       cliente e nel caso in cui il cliente sia autorizzato crea il
       thread lavoratore mittente per tale connessione */
    threadPool_submit(tpool, workerReceiverTask, (void *) asock, NULL, NULL);
  }

  /*
   *   TERMINAZIONE GRADUALE DI TUTTI I THREAD LAVORATORI
   */

  if (dbg) fprintf(stderr, "---DBG--- msgser-main: closing\n");

  _handle_meno1err_ptexit(  unlink(_lsock_path),
			    "main", "unlink"  );
  _handle_meno1err_ptexit(  closeSocket(lsock),
			    "main", "closeSocket"  );

  threadPool_shutdownNow(tpool);
  threadPool_awaitTermination(tpool);

  if (dbg) fprintf(stderr, "msgserv-main: end: fine awaitTermination\n");

  pthread_cancel(writerThr);
  _handle_ptherr_exit(  pthread_join(writerThr, NULL),
			"main", "pthread_join"  );

  free_hashTable(&usrTable);
  blockingList_free(&conntedUsrList);
  blockingList_free(&logStack);

  pthread_cleanup_pop(0);

  {
    struct timespec tosleep;

    tosleep.tv_nsec = 0;
    tosleep.tv_sec = 1;
    nanosleep(&tosleep, NULL);
  }

  if (dbg) fprintf(stderr, "---DBG--- msgserv-main: terminated\n");

  return 0;
}

