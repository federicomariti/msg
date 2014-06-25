/**
   \file msgcli.c
   \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>

#include "errorUtil.h"
#include "comsock.h"

char * userName = NULL;

#ifdef  dbg_msgcli
  static char dbg = 1;
#elif defined dbg_msgcli_
  static char dbg = 2;
#else
  static char dbg = 0;
#endif


#ifdef _notest
  static char notest = 1;
#else
  static char notest = 0;
#endif

#define _exename "msgcli"


/* variabile intera per stabilire la terminazione del processo. e'
scritta dal gestore dei segnali di uscita o dal thread di invio al
server quando riceve la richiesta di uscita da parte dell'utente */
volatile sig_atomic_t leave = 0; /* 0  non terminazione
				  * 1  uscita dovuta all'arrivo di 
				  *    un segnale 
				  * 2  uscita dovuta al verificarsi 
				  *    di un errore 
				  */


#define _sendMessage(socket, msgVar, retVar, errTodo, intrTodo)		\
  if ( 0 > (retVar = sendMessage(sock_fd, &msgVar)) ) {			\
									\
    if ( -1 == retVar ) {						\
      if (EINTR == errno) intrTodo;					\
      fprintf(stderr, _exename": impossibile spedire: %s\n",		\
	      strerror(errno));						\
    } else if ( SEOF == retVar ) {					\
      fprintf(stderr, _exename": il server non e' piu' connesso\n");	\
    }									\
									\
    errTodo;								\
  }									

#define _receiveMessage(socket, msgVar, retVar, errTodo, intrTodo)	\
  if ( 0 > (retVar = receiveMessage(sock_fd, &msgVar)) ) {		\
									\
    if ( -1 == retVar ) {						\
      if (EINTR == errno) {                                             \
	intrTodo;                                                       \
      }					                                \
      fprintf(stderr, _exename": impossibile spedire: %s\n",		\
	      strerror(errno));						\
    } else if ( SEOF == retVar ) {					\
      fprintf(stderr, _exename": il server non e' piu' connesso\n");	\
    }									\
									\
    errTodo;								\
  }									


/* ---- vv readLine vv -------------------------------------------------- */

#define _bufsz 8

/** legge una riga dal file fl memorizzando i caratteri in
 *  memoria heap terminati dal carattere di fine stringa '\0',
 *  modifica il valore del puntatore riferito dal parametro strref con
 *  l'indirizzo base di memoria heap dove la riga e' stata salvata.
 *
 *  \param  fd      file descrtiptor del file da cui leggere
 *
 *  \param  strref  riferimento alla variabile in cui salvare
 *                  l'indirizzo base di memoria heap dove la riga e' stata
 *                  salvata terminata da '\0'
 *
 *  \retval   n     numero di caratteri letti
 *
 *  \retval   -1    si e' verififcatp un errore, variabile errno
 *                  settata, l'eventuale memoria occupata e' liberata
 *                  e la variabile riferita da strref e' impostata a
 *                  NULL, 
 *                  oppure e' occorso EOF senza alcun carattere letto,
 *                  in questo caso la variabile errno non e' settata
 *
 */
int 
readLine(FILE * fl, char ** strref)
{
  int bfsz = _bufsz, bfactlsz = 0, unused = 0;
  char exit = 0, * tmp = NULL;

  if (NULL == fl  || NULL == strref) {
    errno = EINVAL;
    return -1;
  }

  if (*strref != NULL) *strref = NULL;

  bfactlsz = bfsz;
  
  while (!exit) 
  {
    if( NULL == (tmp = realloc(*strref, bfactlsz)) ) {
      free(*strref); *strref = NULL;
      return -1; /* oom */
    }
    *strref = tmp; 
    bzero((*strref)+bfactlsz-bfsz, bfsz);
  
    if ( NULL == fgets(*strref+bfactlsz-bfsz-unused, bfsz+unused, fl) ) {
      /* si e' verificato un errore (errno settata) oppure e' occorso
	 EOF prima di leggere nessun carattere */

      return -1; 
    }

    tmp = *strref+bfactlsz-bfsz-unused;
    while (!exit  &&  tmp < *strref + bfactlsz)
    {
      if ('\n' == *tmp) {
	exit = 1;
      } else
	tmp++;
    }

    bfactlsz += bfsz;
    unused = 1;
  } 
  
  *tmp = '\0';
  unused = (*strref)+bfactlsz-bfsz - tmp-1;
  if ( NULL == (tmp = realloc(*strref, bfactlsz-bfsz -  unused)) ) {
    free(strref); *strref = NULL;
    return -1;
  }

  return bfactlsz-bfsz-unused-1;
}

/** legge una riga da standard input memorizzando i caratteri in
 *  memoria heap terminati dal carattere di fine stringa '\0',
 *  modifica il valore del puntatore riferito dal parametro strref con
 *  l'indirizzo base di memoria heap dove la riga e' stata salvata.
 *
 *  la riga viene letta completamente anche in caso di arrivo di segnali
 *
 *  in caso di cancellazione ?
 *
 *  \param  fd      file descrtiptor del file da cui leggere
 *
 *  \param  strref  riferimento alla variabile in cui salvare
 *                  l'indirizzo base di memoria heap dove la riga e' stata
 *                  salvata terminata da '\0'
 *
 *  \retval   n     numero di caratteri letti
 *
 *  \retval   -1    si e' verififcatp un errore, variabile errno
 *                  settata , l'eventuale memoria occupata e' liberata
 *                  e la variabile riferita da strref e' impostata a
 *                  NULL
 *
 */
int 
readLine_(int fd, char ** strref)
{
  int bytesRead = 0, bfsz = _bufsz, bfactlsz = 0;
  
  if (fd < 0  || NULL == strref) {
    errno = EINVAL;
    return -1;
  }

  bfactlsz = bfsz;
  
  if( NULL == (*strref = malloc(bfsz)) ) {
    *strref = NULL;
    return -1; /* oom */
  }
  
  errno = 0;
  while ( ( 0 < (bytesRead = read(fd, *strref+bfactlsz-bfsz, bfsz)) 
	    &&  bytesRead == bfsz
	    &&  (*strref)[bfactlsz-1] != '\n' )
       /* ||  (-1 == bytesRead  &&  EINTR == errno) */)
  {
    if (EINTR == errno) continue; 
    bfactlsz += bfsz;
    if ( NULL == (*strref = realloc(*strref, bfactlsz)) ) {
      free(*strref);
      *strref = NULL;
      return -1; /* oom */
    }
    /*
    printf("bytesRead = %d, buffer = >%s<\n", bytesRead, *strref);
    */
  }

  if (EINTR == errno) {
    free(*strref);
    return -1;
  }

  bfactlsz -= bfsz - bytesRead;

  if ( '\n' == (*strref)[bfactlsz-1] ) (*strref)[bfactlsz-1] = '\0';
  else  ; /* ??? */

  if ( NULL == (*strref = realloc(*strref, bfactlsz)) ) {
    free(*strref);
    *strref = NULL;
    return -1; /* oom */
  }
  /*
  printf("bytesRead = %d, buffer = >%s<, bufferSize = %d\n", 
	 bytesRead, *strref, bfactlsz);
  */
  return bfactlsz - 1;
}

/* ---- ^^ readLine ^^ -------------------------------------------------- */

void
senderTask_cleanup(void * arg)
{
  char ** line = (char **) arg;

  free(*line);
}

void *
senderTask(void * arg)
{
  /* file descriptor del socket connessso al server */
  int sock_fd = (int) arg;

  int localLeave = 0;           /* 0 non terminazione
				 * 1 terminazione richiesta dal client
				 * 2 terminazione dovuta ad un errore 
				 */
  message_t nextMsg;
  int msglen = 0, err = 0;
  char * nextLine = NULL, * msgtype = NULL, * msgbuffer = NULL;

  if (dbg) fprintf(stderr, "---DBG--- msgcli-sender: start, socket = %d\n", 
		   sock_fd);

  pthread_cleanup_push(senderTask_cleanup, &nextLine);

  while (!localLeave  &&  !leave) 
  {
    /*
     *   LEGGI DA STANDARD INPUT
     */
    if ( 1 > (msglen = readLine(stdin, &nextLine)) ) { /* BLOCKING */
      if ( EINTR == errno  || 0 == msglen ) continue;
      
      if ( errno ) perror(_exename": readLine");
      else ; /* EOF, do nothing, exit */

      localLeave = 2;
      continue;
    }

    if ('%' == *nextLine) { 
      msgtype = strtok(nextLine+1, " ");
      msgbuffer = strtok(NULL, "");
      msglen =  msglen - strlen(msgtype) - 2 + 1; /* meno il tipo, meno i caratteri '%' e ' 'm piu' il carattere '\0' */
    } else {
      /* il messaggio e' di broadcast */
      msgtype = NULL;
      msgbuffer = nextLine; 
      msglen++; /* piu' il carattere '\0' */
    }


    if (dbg) fprintf(stderr, "---DBG--- msgcli-sender: TO SEND: type = >%s<,  message = >%s<,"
	   "message length = %d\n", msgtype, msgbuffer, msglen);


    /*
     *   ESEGUI LA RICHIESTA 
     */
    if ( NULL == msgtype ) { /* BROADCAST MESSAGE */
      
      _createBcastMesssage_length(nextMsg, msgbuffer, msglen);
      _sendMessage(sock_fd, nextMsg, err,
		   free(nextLine); localLeave = 2; continue,
		   free(nextLine); continue);		   
					     
    } else if ( !strcasecmp("one", msgtype) ) { /* TO ONE MESSAGE */
      
      int i = 0;
      while (' ' != msgbuffer[i]  
	     && '\0' != msgbuffer[i] ) i++;
      
      if ( '\0' != msgbuffer[i] ) {
	msgbuffer[i] = '\0';
	if (2==dbg) fprintf(stderr, "msgcli-sender: message_str = %s\\0%s\n",
			 msgbuffer, msgbuffer+i+1);
	_createToOneMessage_length(nextMsg, msgbuffer, msglen);
	_sendMessage(sock_fd, nextMsg, err,
		     free(nextLine); localLeave = 2; continue,
		     free(nextLine); continue);
      }	  

    } else if ( !strcasecmp("list", msgtype) ) { /* LIST MESSAGE */
      
      _createListRequest(nextMsg);
      _sendMessage(sock_fd, nextMsg, err,
		   free(nextLine); localLeave= 2; continue,
		   free(nextLine); continue);		   

    } else if ( !strcasecmp("exit", msgtype) ) { /* EXIT MESSAGE */
      
      localLeave = 1;
      
    } else { /* INCORRECT MESSAGE */
      
      if (notest) printf(_exename": \"%s\" tipo di messaggio sconosciuto\n", msgtype);
      
      ; /* do nothing */
    }

    if (dbg) fprintf(stderr, "---DBG--- msgcli-sender: SENT: type = %s\n", msgtype);

    free(nextLine); 
    nextLine = NULL;  

  } /* while (!localLeave  &&  !leave) */

  pthread_cleanup_pop(leave);



  if (dbg) fprintf(stderr, "---DBG--- msgcli-sender: end, "
		   "leave = %d, localLeave = %d\n", leave, localLeave);

  return (void *) 0;
}

void
receiverTask_cleanup(void * arg)
{
  if (2==dbg) fprintf(stderr, "---DBG--- receiverTask_cleanup: start\n");
  ;
}

void *
receiverTask(void * arg)
{
  /* file descriptor del socket connesso al server */
  int sock_fd = (int) ((void **) arg) [0];

  /* identificatore del thread invia messaggi dal server, serve per la
     cancellazionein caso di shutdown del server*/
  pthread_t * senderThread = (pthread_t *) ((void **) arg) [1];

  /* indica lo stato di terminazione
   * 0 non terminazione
   * 1 terminazione confermata dal server
   * 2 si e' verificato un errore
   */
  char localLeave = 0;    
  sigset_t mySigmask_ss;
  message_t nextMsg;
  int err = 0;

  /*
   *   FILTRO TUTTI I SEGNALI 
   */

  sigfillset(&mySigmask_ss);

  if ( 0 != pthread_sigmask(SIG_SETMASK, &mySigmask_ss, NULL) ) {
    perror(_exename": errore non gestibile");
    perror("receiverTask: pthread_sigmask");
    if (dbg) fprintf(stderr, 
		     _exename": receiverThread: pthread_sigmask: %s", 
		     strerror(errno)); 
    return NULL;
  }       

  pthread_cleanup_push(receiverTask_cleanup, NULL);

  if (dbg) fprintf(stderr, "---DBG--- msgcli-receiver: start\n");
  
  /*
   *   RICEVO MESSAGGI DAL SERVER
   */
  while (!localLeave  &&  !leave)
  {
    nextMsg.type = '.'; nextMsg.length = 0; nextMsg.buffer = NULL;

    errno = 0;
    if ( 0 > (err = receiveMessage(sock_fd, &nextMsg)) ) {
      localLeave = 2;
      if ( -1 == err ) {
	if (errno) perror("receiverTask: receiveMessage");
	else fprintf(stderr, "receiverTask: receiveMessage: error\n");
      } else if ( SEOF == err ) {
	fprintf(stderr, "Connection is down.\n");
      }
      break;
    }

    if (dbg) fprintf(stderr, "---DBG--- msgcli-receiver: RECEIVED: type = %c, "
		     "buffer = >%s<\n", nextMsg.type, nextMsg.buffer);

    switch (nextMsg.type) {
    case MSG_ERROR:
      printf("[ERROR] ");
      break;
    case MSG_EXIT:
      printf("The server is shutdown. exit.\n");
      if ( -1 == pthread_detach(pthread_self()) ) 
	perror("receiverTask: pthread_detach");
      pthread_cancel(*senderThread);
      fflush(stdout);
      fflush(stderr);

      if (dbg) fprintf(stderr, "---DBG--- msgcli-receive: terminated by server\n");

      return (void *) 0;
    case MSG_OK:
      /* l'uscita richiesta precedentemente e' stata confermata da
	 parte del server */
      localLeave = 1;
      break;
    case MSG_LIST:
      printf("[LIST] ");
      break;
    case MSG_BCAST:
      printf("[BCAST]");
      break;
    case MSG_TO_ONE:
      ;
      break;
    default:
      printf("Received unknown message from server\n");
    }

    if ( nextMsg.length  &&  nextMsg.buffer ) {
      printf("%s\n", nextMsg.buffer);
      free(nextMsg.buffer);
    }
  
  } /* while (!localLeave  &&  !leave); */

  pthread_cleanup_pop(0);

  if ( 2 == localLeave ) {
    /* uscita dovuta al verificarsi di un errore nell'esecuzione del
       thread corrente, occorre cancellare il thread che invia
       messaggi al server */
    pthread_cancel(*senderThread);
  }

  if (dbg) fprintf(stderr, "---DBG--- msgcli-receiver: terminated\n");

  fflush(stdout);
  fflush(stderr);

  return (void *) 0;
}


void
signalHandler_exit(int sig)
{
  write(2, "msgcli: Ricevuto un segnale, esco.\n", 35);
  leave = 1;
}


void
signalHandler_ignoreIt(int sig)
{
  write(2, "msgcli: Ricevuto un segnale, ignorato.\n", 39);
  ; 				/* DO NOTHING */
}

#define _maxLengthUserName 256
#define _serversock_path "/tmp/msgsock"

int
main(int argc, char ** argv)
{
  /* mappe di segnali */
  sigset_t allSignal_ss, oldSignalConfig_ss;
  
  /* azioni su segnali */
  struct sigaction ignore_sa, exit_sa;

  int sock_fd = -1, err = 0;

  message_t mainMsg;

  pthread_t self;

  /* identificatore del thread che riceve messaggi dal server */
  pthread_t receiverThread;
  void * receiverTaskArg[2];

  if (argc != 2)
    return 0;

  userName = argv[1];

  /* legalita' nome utente .. */

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

  /* ripristiono la signal mask */
  
  _handle_ptherr_exit(  pthread_sigmask( SIG_SETMASK, 
					 &oldSignalConfig_ss, NULL), 
			"main", "pthread_sigmask-1"  );

  /*
   *   TENTA LA CONNESSIONE AL SERVER
   */
  if (notest) printf(_exename": Connessione al server %s\n", _serversock_path);

  if ( -1 == (sock_fd = openConnection(_serversock_path)) ) { /* BLOCKING */
    fprintf(stderr, _exename": Impossibile stabilire la connessione "
	    "con il server %s: %s\n", _serversock_path, strerror(errno));
    return -1;
  }

  _createConnMessage(mainMsg, userName);
  _sendMessage(sock_fd, mainMsg, err,
	       closeSocket(sock_fd); return -1,
	       closeSocket(sock_fd); return -1);
  _receiveMessage(sock_fd, mainMsg, err, 
		  closeSocket(sock_fd); return -1,
		  _createExitMessage(mainMsg); 
		  _sendMessage(sock_fd, mainMsg, err, 
			       closeSocket(sock_fd); return -1,
			       closeSocket(sock_fd); return -1);
		  closeSocket(sock_fd); return -1);

  if (MSG_ERROR == mainMsg.type) {
    if (0 < mainMsg.length) {
      fprintf(stderr, _exename": %s\n", mainMsg.buffer);
      free(mainMsg.buffer);
    } else 
      fprintf(stderr, _exename": Il server ha rifiutato la richiesta di "
	      "connessione. Esco.\n");
    
    closeSocket(sock_fd);
    return -1;
  } 

  if (MSG_NO == mainMsg.type) {
    printf(_exename": Il server ha rifiutato la richiesta di connessione. Esco.\n");

    closeSocket(sock_fd);
    return -1;
  }

  if (MSG_OK != mainMsg.type) {
    printf(_exename": Risposta dal server non valida. Esco.\n");
    closeSocket(sock_fd);
    return -1;      
  }

  if (notest) printf(_exename": Connesso al server %s\n", _serversock_path);

  /*
   *   AVVIA IL THREAD CHE RICEVE MESSAGGI DAL SERVER
   */ 
  self = pthread_self();
  receiverTaskArg[0] = (void *) sock_fd;
  receiverTaskArg[1] = &self;
  if ( 0 != (errno = pthread_create(&receiverThread, NULL,
				    receiverTask, receiverTaskArg)) )
    perror("main: pthread_create");

  /*
   *   RIMANI IN ATTESA DI MESSAGGI DALL'UTENTE E INVIA AL SERVER
   */
  senderTask((void *) sock_fd);

  /*
   *   CHIUSURA: INVIA IL MESSAGGIO DI USCITA AL SERVER 
   *             (solo se non si e' ricevuto il segnale di terminazione)
   */
  _createExitMessage(mainMsg);
  _sendMessage(sock_fd, mainMsg, err,
	       fprintf(stderr, "Impossibile inviare il messaggio di chiusura al server\n");,
	       ;);

  /*
   *   CHIUSURA: ASPETTA LA TERMINAZIONE DEL THREAD CHE RICEVE DAL SERVER
   */
  if ( 0 != (errno = pthread_join(receiverThread, NULL)) )
    perror("main: pthread_join");

  /* 
   *   CHIUSURA: CHIUDI IL SOCKET
   */
  if ( -1 == closeSocket(sock_fd) )
    perror("main: closeSocket");

  fflush(stdout);
  fflush(stderr);

  if (dbg) fprintf(stderr, "---DBG--- msgcli-main: end\n");


  return 0;
}
/*
Il client `e un comando Unix che viene attivato da linea di comando con 
$ ./msgcli username 
Appena attivato, il client eﬀettua il parsing delle opzioni, e se il parsing `e 
  corretto cerca di collegarsi con il server (su ./tmp/msgsock) per un massimo di 5 
tentativi a distanza di 1 secondo l’uno dell’altro. Se il collegamento ha successo 
invia un messaggio di CONNECT per collegarsi. Se il collegamento `e accettato dal 
server il client crea i propri thread e si mette in attesa di messaggi da inviare 
  (su standard input) o di messaggi da altri utenti da visualizzare (dal server). 
  Processo msgcli `e costituito da due thread paralleli: 
  • un thread che si occupa di leggere i messaggi da inviare da standard input, 
di veriﬁcarne la correttezza e di inviarli al server 
• un thread che ascolta i messaggi in arrivo dalla socket di connessione con 
il server e li visualizza sullo standard output 
Il formato con cui l’utente eﬀettua richieste sullo standard input e’ il seguen- 
te. Per i messaggi in broadcast e’ suﬃciente speciﬁcare il testo del messaggio 
    terminato da newline \n es: 
ciao a tutti! 
il messaggio o puo’ contenere qualsiasi carattere eccetto % che serve per intro- 
durre gli altri comandi che sono 
%LIST 
  per avere la lista di tutti gli utenti connessi, 
%EXIT 
per terminare la sessione ed il processo client e 
%ONE 
  per inviare un messaggi ad un singolo utente, come ad esempio in 
5
%ONE pippo ciao! 
  in questo caso la prima parola dopo %ONE (separatore ’ ’ spazio) viene in- 
  terpretata come nome dell’utente e il resto (ﬁno al newline) come testo del 
messaggio. 
I messaggi in arrivo dal server vengono visualizzati sullo standard output in 
  ordine di arrivo preceduti dal loro tipo, ad esempio: 
[LIST] paperino pluto pippo 
  visualizza gli utenti connessi (preceduti da [LIST] e separati da spazi). Mentre 
  i messagi in broadcast vengono visualizzati come: 
[BCAST][pippo] Ciao a tutti! 
e i messaggi da uno speciﬁco utente come 
[pippo] Ciao! 
dove il nome dell’utente da cui arriva il messaggio viene visualizzato fra parentesi 
quadre. Notare che la visualizzazione di ogni messaggio e’ terminato da newline 
\n. 
L’inserimento di un comando scorretto provoca la stampa di un breve mes- 
saggio di uso che riassume i comandi e la loro semantica. Il formato di questo 
messaggio e’ lasciato allo studente. 
Il processo pu` 
o essere terminato gentilmente inviando un segnale di SIGTERM 
o SIGINT. All’arrivo di questo segnale il processo deve visualizzare i messaggi 
  gia’ ricevuti, informare il server della terminazione e terminare gentilmente. 
*/
