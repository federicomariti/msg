/**
 *  \file myTest_comsock.c
 *  \author lso10
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "errorUtil.h"
#include "comsock.h"

#define _proc_name "myTestComsock"
#define _serverSock_name "/tmp/test_comsock_socket"

#ifdef DBG
static const char dbg = 1;
#else
static const char dbg = 0;
#endif

#define _printDbg_tt(thread_name, text)		\
  if (dbg) printf("--- DBG:  "thread_name": "text"  ---\n");

#define _handle_socketEOF(info)			\
  if (-2 == err) {				\
    err = errno;				\
    fprintf(stderr, info);			\
    pthread_exit((void *) err);			\
  }

#define _sleep(_timespec_var, _toSleep, _threadName)			\
  _timespec_var.tv_sec = _toSleep;					\
  _handle_meno1err_ptexit(  nanosleep(&_timespec_var, NULL),		\
			    _threadName, "nanosleep"  );		


/* ====== vv simpleTask2 vv ======================================== */

#define _sst2_bufferSize 32

void
simpleServer2_clup(void *arg)
{
  int * sock_fd = (int *) ((void **) arg) [0]; /* sock di ascolto */
  int * asock_fd = (int *) ((void **) arg) [1]; /* sock attivo */
  char ** sock_name = (char **) ((void **) arg) [2]; 
  char ** buffer = (char **) ((void **) arg) [3];

  if (-1 != *sock_fd  &&  -1 == close(*sock_fd))
    perror("simpleServer2_clup: close-lsock");
  
  if (-1 != *asock_fd  &&  -1 == close(*asock_fd))
    perror("simpleServer2_clup: close-asock");
  
  if (NULL != *sock_name  &&  unlink(*sock_name))
    perror("simpleServer2_clup: unlink");

  if (*buffer != NULL)
    free(*buffer);

  _printDbg_tt("simpleServer2_clup", "chiuso");
}

void *
simpleServerTask2(void * arg)
{
  int preCreateSleep = (int) ((void **) arg) [0];
  int preAcceptSleep = (int) ((void **) arg) [1];
  int postAcceptSleep = (int) ((void **) arg) [2];
  int sock_fd = -1;
  int asock_fd = -1;
  struct timespec toSleep;
  message_t rcvedMsg;
  int err = 0;
  void * cleanupArg[3];
  char * lsock_name = NULL;

  rcvedMsg.buffer = NULL;

  toSleep.tv_nsec = 0;

  cleanupArg[0] = &sock_fd; cleanupArg[1] = &asock_fd;
  cleanupArg[2] = &lsock_name; cleanupArg[3] = &rcvedMsg.buffer;
  pthread_cleanup_push(simpleServer2_clup, cleanupArg);
	

  _printDbg_tt("simpleServer", "iniziato");

  _sleep(toSleep, preCreateSleep, "simpleServer");

  
  _handle_meno1err_ptexit(  sock_fd = 
			    createServerChannel(_serverSock_name),
			    "simpleServer", "createServerChannel"  );

  lsock_name = _serverSock_name; /* per la funzione di clean up */

  _printDbg_tt("simpleServer", "creato un nuovo canale");

  _sleep(toSleep, preAcceptSleep, "simpleServer");


  _printDbg_tt("simpleServer", "aspetto richieste di connessione");

  _handle_meno1err_ptexit(  asock_fd = 
			    acceptConnection(sock_fd),
			    "simpleServer", "acceptConnection"  );

  _printDbg_tt("simpleServer", "accettata una richiesta d connessione");
  
  _sleep(toSleep, postAcceptSleep, "simpleServer");

      
  _handle_meno1err_ptexit(  err = 
			    receiveMessage(asock_fd, &rcvedMsg),
			    "simpleServer", "receiveMessage"  );

  _handle_socketEOF("simpleServer: receive: il client ha chiuso la connessione, chiudo.\n");

  printf("simpleServer: messaggio ricevuto: type=%c, length=%d, value=%s\n",
	 rcvedMsg.type, rcvedMsg.length, rcvedMsg.buffer);


  _handle_meno1err_ptexit(  err =
			    sendMessage(asock_fd, &rcvedMsg),
			    "simpleServer", "sendMessage"  );

  _handle_socketEOF("simpleServer: send: il client ha chiuso la connessione, chiudo.\n");
  


  free(rcvedMsg.buffer);
			      
  _handle_meno1err_ptexit(  closeSocket(sock_fd),
			    "simpleServer", "close"  );
  _handle_meno1err_ptexit(  closeSocket(asock_fd),
			    "simpleServer", "close"  );
  _handle_meno1err_ptexit(  unlink(lsock_name),
			    "simpleServer", "unlink"  );

  _printDbg_tt("simpleServer", "chiuso il canale");


  pthread_cleanup_pop(0);
	
  return (void *) 0;
}
  
#define _sct2_bufferSize 32

void *
simpleClientTask2(void * arg)
{
  int sock_fd;
  int preOpenSleep = (int) ((void **) arg) [0];
  int preCloseSleep = (int) ((void **) arg) [1];
  struct timespec toSleep;
  message_t toSnd_msg;
  int err = 0;

  toSleep.tv_nsec = 0;


  _printDbg_tt("simpleClient", "iniziato");

  _sleep(toSleep, preOpenSleep, "simpleClient");


  _handle_meno1err_ptexit(  sock_fd =
			    openConnection(_serverSock_name),
			    "simpleClient", "openConnection"  );

  _printDbg_tt("simpleClient", "connesso al server");


  _createToOneMessage(toSnd_msg, "The Great Gig In The Sky");

  _handle_meno1err_ptexit(  err =
			    sendMessage(sock_fd, &toSnd_msg),
			    "simpleClient", "sendMessage"  );

  _handle_socketEOF("simpleClient: send: il server ha chiuso il canale, chiudo\n");

  _printDbg_tt("simpleClient", "inviato un messaggio");


  bzero(&toSnd_msg, sizeof(toSnd_msg));

  _handle_meno1err_ptexit(  err = 
			    receiveMessage(sock_fd, &toSnd_msg),
			    "simpleClient", "receiveMessage"  );

  _handle_socketEOF("simpleClient: receive: il server ha chiuso il canale, chiudo\n");

  printf("simpleClient: messaggio ricevuto: typ=%d, lng=%d, vl=%s\n", 
	 toSnd_msg.type, toSnd_msg.length, toSnd_msg.buffer);

  free(toSnd_msg.buffer);


  _sleep(toSleep, preCloseSleep, "simpleClient");

  _handle_meno1err_ptexit(  closeSocket(sock_fd),
			    "simpleClient", "closeSocket"  );

  _printDbg_tt("simpleClient", "chiuso il socket");

  
  return (void *) 0;
}

/* ====== ^^ simpleTasks2 ^^ ==================== vv simpleTasks vv ====== */

void *
simpleServerTask(void * arg)
{
  int preCreateSleep = (int) ((void **) arg) [0];
  int preAcceptSleep = (int) ((void **) arg) [1];
  int postAcceptSleep = (int) ((void **) arg) [2];
  int sock_fd;
  struct timespec toSleep;

  _printDbg_tt("simpleServer", "iniziato");

  toSleep.tv_nsec = 0;

      toSleep.tv_sec = preCreateSleep;
      _handle_meno1err_ptexit(  nanosleep(&toSleep, NULL),
				"simpleServer", "nanosleep"  );

  _handle_meno1err_ptexit(  sock_fd = 
			    createServerChannel(_serverSock_name),
			    "simpleServer", "createServerChannel"  );
  _printDbg_tt("simpleServer", 
	       "creato un nuovo canale");

      toSleep.tv_sec = preAcceptSleep;
      _handle_meno1err_ptexit(  nanosleep(&toSleep, NULL),
				"simpleServer", "nanosleep"  );  
    
  _printDbg_tt("simpleServer", 
	       "aspetto richieste di connessione");
  _handle_meno1err_ptexit(  acceptConnection(sock_fd),
			    "simpleServer", "acceptConnection"  );
  _printDbg_tt("simpleServer",
	       "accettata una richiesta d connessione");
  
      toSleep.tv_sec = postAcceptSleep;
      _handle_meno1err_ptexit(  nanosleep(&toSleep, NULL),
				"simpleServer", "nanosleep"  );



  _handle_meno1err_ptexit(  closeSocket(sock_fd),
			  "simpleServer", "close"  );
  _handle_meno1err_ptexit(  unlink(_serverSock_name),
			  "simpleServer", "unlink"  );
  _printDbg_tt("simpleServer", "chiuso il canale");

	
  return (void *) 0;
}
  

void *
simpleClientTask(void * arg)
{
  int sock_fd;
  int preOpenSleep = (int) ((void **) arg) [0];
  int preCloseSleep = (int) ((void **) arg) [1];
  struct timespec toSleep;

  _printDbg_tt("simpleClient", "iniziato");

  toSleep.tv_nsec = 0;

      toSleep.tv_sec = preOpenSleep;
      _handle_meno1err_ptexit(  nanosleep(&toSleep, NULL),
				"simpleServer", "nanosleep"  );

  _handle_meno1err_ptexit(  sock_fd = openConnection(_serverSock_name),
			    "simpleClient", "openConnection"  );
  _printDbg_tt("simpleClient", "connesso al server");

      toSleep.tv_sec =  preCloseSleep;
      _handle_meno1err_ptexit(  nanosleep(&toSleep, NULL),
				"simpleServer", "nanosleep"  );

  _handle_meno1err_ptexit(  closeSocket(sock_fd),
			    "simpleClient", "closeSocket"  );
  _printDbg_tt("simpleClient", "chiuso il socket");

  
  return (void *) 0;
}

/* ====== ^^ simpleTasks1 ^^ ==================== vv simpleTest vv ====== */

int 
simpleTest(int s_preCrt, int s_preAcp, int s_postApt, 
	   int c_preOpn, int c_preCls, int todo)
{
  pthread_t server_tid, client_tid;
  void * server_es, * client_es;
  int serverArg[3], clientArg[2];
  void * (* clientTask) (void *);
  void * (* serverTask) (void *);

  serverArg[0] = s_preCrt; serverArg[1] = s_preAcp; serverArg[2] = s_postApt;
  clientArg[0] = c_preOpn; clientArg[1] = c_preCls;

  if (!todo) {
    clientTask = simpleClientTask; serverTask = simpleServerTask;
  } else {
    clientTask = simpleClientTask2; serverTask = simpleServerTask2;
  }

  _handle_ptherr_exit(  pthread_create(&server_tid, NULL,
				       serverTask, serverArg),
				       _proc_name, "pthread_create-1"  );
  _handle_ptherr_exit(  pthread_create(&client_tid, NULL,
				       clientTask, clientArg),
			_proc_name, "pthread_create-2"  );

  _handle_ptherr_exit(  pthread_join(server_tid, &server_es),
			_proc_name, "pthread_join-1"  );
  _handle_ptherr_exit(  pthread_join(client_tid, &client_es),
			_proc_name, "pthread_join-2"  );


  return !((int)server_es == 0 && (int)client_es == 0 );
}

/* create channal and close */
int
sngTh_2(void)
{
  
  return 0;
}
 

/* create channal, accept connection, open connection,
   close */
int
singleTh_2(void)
{
  
  return 0;
}



#define _test(text, exp, es_int)		\
  printf(text"\n");				\
  if ( ( es_int = ( exp ) ? -1 : es_int ) )	\
    printf("\n    X  fallito\n");		\
  else						\
    printf("\n    V  completato\n");


int
main(int argc, char ** argv)
{
  int es = 0;
  int min = 0, max = 7, i = 0;

  switch (argc) {
  case 1+1:
    min = (int)*argv[1] - 48;
    max = min;
    break;
  case 2+1:
    min = (int)*argv[1] - 48; 
    max = (int)*argv[2] - 48;
    break;    
  }

  for (i=min; i<max+1; i++)
    switch (i) {
      /*                   s e r v e r      client
                      prCrt prApt poApt   prCnt prCl
      */
    case 0:
      /* client effettua un tentativo prima di connettersi */
      _test("simple test 0: client effettua un tentativo prima di connettersi",
	    simpleTest(0,    0,    0,       1,    0,  0), es); break;
    case 1:  
      /* client effettua due tentativi prima di connettersi */
      _test("simple test 1: client effettua due tentativi prima di connettersi",
	    simpleTest(1,    0,    0,       0,    0,  0), es); break;
    case 2:
      /* client effettua tre tentativi prima di connettersi */
      _test("simple test 2: client effettua tre tentativi prima di connettersi",
	    simpleTest(2,    0,    0,       0,    0,  0), es); break;
    case 3:
      /* client non trova il server in attesa */
      _test("simple test 3: client non trova il server in attesa",
	    simpleTest(3,    0,    0,       0,    0,  0), es); break;
      /* blocca, client: openconn: ETIMEDOUT */
    case 4:
      /* il server binda e imposta di ascolto il socket ma non accetta */
      break;
    case 5:
      /* il client chiude la connessione prima del server */
      _test("simple test 5: il client chiude la connessione prima del server",
	    simpleTest(0,    0,    5,       1,     0,  0), es); break;
    case 6:
      /* il server chiude la connessione prima del client */
      _test("simple test 6: il server chiude la connessione prima del client", 
	    simpleTest(0,    0,    0,       1,     5,  0), es); break;      
    case 7:
      _test("simple test 7: echo server", 
	    simpleTest(0,    0,    0,       1,     3,  1), es); break;
    }

  return 0;
}
/*
i=0, conn
1 sec
i=1, conn
1 sec
i=2, conn
*/
  
