/**
 *  \file comsock.c
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/un.h>
#include <time.h>

#include "comsock.h"


void * 
message_copy(void * a)
{
  message_t * _a = NULL;
  
  if ( NULL == (_a = malloc(sizeof(message_t))) ) {
    return NULL;
  }

  _a->type   = ((message_t *) a)->type;
  _a->length = ((message_t *) a)->length;

  if ( 0 < _a->length ) {

    if ( NULL == (_a->buffer = malloc(_a->length)) ) {
      free(_a);
      return NULL;
    }
    strncpy(_a->buffer, ((message_t *) a)->buffer, _a->length);

  } else {

    _a->buffer = NULL;

  }    
  
  return (void *) _a;
}

int 
message_compare(void * a, void * b) /* TODO */
{
  return (int) a - (int) b;
}



/** Crea una socket AF_UNIX
 *  \param  path pathname della socket
 *
 *  \retval s    il file descriptor della socket  (s>0)
 *  \retval SNAMETOOLONG se il nome del socket eccede UNIX_PATH_MAX
 *  \retval -1   in altri casi di errore (sets errno)
 *
 *  in caso di errore ripristina la situazione inziale: rimuove
 *  eventuali socket create e chiude eventuali file descriptor rimasti
 *  aperti
 */
int 
createServerChannel(char* path)
{
  int result = 0;
  struct sockaddr_un sockAddr;

  /* verifica la correttezza dei parametri in ingresso */
  if (NULL == path) {
    errno = EINVAL; return -1;
  }
  if (SNAMETOOLONG < strlen(path)) {
    errno = EINVAL; return SNAMETOOLONG;
  }

  /* crea il socket di tipo stream */
  if (-1 == (result = socket(AF_UNIX, SOCK_STREAM, 0)))
    return -1; /* errno is set by function */
    
  /* associa un nome al socket */
  bzero(&sockAddr, sizeof(struct sockaddr_un));
  sockAddr.sun_family = AF_UNIX;
  strncpy(sockAddr.sun_path, path, sizeof(sockAddr.sun_path));
  if (-1 == bind(result, (struct sockaddr *) &sockAddr, 
		 sizeof(struct sockaddr_un))) {
    close(result);
    return -1; /* errno is set by function */
  }

  /* imposta il socket come di ascolto */  
  if (-1 == listen(result, SOMAXCONN)) {
    unlink(path);
    close(result); 
    return -1; /* errno is set by function */
  }

  return result;  
}

/** Chiude una socket
 *   \param s file descriptor della socket
 *
 *   \retval 0  se tutto ok, 
 *   \retval -1  se errore (sets errno)
 */
int 
closeSocket(int s) 
{
  if (0 > s) {
    errno = EINVAL;
    return -1;
  }

  return close(s);
}

/** accetta una connessione da parte di un client
 *  \param  s socket su cui ci mettiamo in attesa di accettare la connessione
 *
 *  \retval  c il descrittore della socket su cui siamo connessi
 *  \retval  -1 in casi di errore (sets errno)
 */
int 
acceptConnection(int s)
{
  struct sockaddr_un addr; socklen_t len = sizeof(addr);
  int result;

  if (0 > s) {
    errno = EINVAL;
    return -1;
  }

  if (-1 == (result = accept(s, (struct sockaddr *) &addr, &len))) 
    return -1;

  return result;
}
    
#define _buffer_size 8
#define _handle_readError				\
  if (ECONNRESET == errno || ENOTCONN == errno)		\
    return SEOF;					\
  else							\
    return -1;						\

/** legge un messaggio dalla socket
 *  \param  sc  file descriptor della socket
 *  \param msg  struttura che conterra' il messagio letto 
 *		(deve essere allocata all'esterno della funzione,
 *		tranne il campo buffer)
 *
 *  \retval lung  lunghezza del buffer letto, se OK 
 *  \retval SEOF  se il peer ha chiuso la connessione 
 *                   (non ci sono piu' scrittori sulla socket)
 *  \retval  -1    in tutti gl ialtri casi di errore (sets errno)
 *      
 */
int
receiveMessage(int sc, message_t * msg)
{
  int bytesRead = 0;

  /* verifico correttezza dei parametri */
  if (0 > sc || NULL == msg) {
    errno = EINVAL;
    return -1;
  }

  
  /* leggo i dati dal socket secondo il formato del protocollo di comunicazione */
  if ( -1 == (bytesRead = read(sc, &msg->type, sizeof(char))) 
       || sizeof(char) != bytesRead ) {
    _handle_readError;
  }
  if ( -1 == (bytesRead = read(sc, &msg->length, sizeof(unsigned int)))
       || sizeof(unsigned int) != bytesRead ) {
    _handle_readError;
  }
  if (msg->length > 0) {

    if ( NULL == (msg->buffer = malloc(msg->length)) ) {
      return -1;
    }

    if ( -1 == (bytesRead = read(sc, msg->buffer, msg->length)) 
	 || msg->length != bytesRead ) {
      free(msg->buffer);
      _handle_readError;
    }    
  }


  return bytesRead;
}
  
    
#define _handle_writeEOF				\
  if (ECONNRESET == errno)				\
    return SEOF;					\
  else							\
    return -1;						\

/** scrive un messaggio sulla socket
 *   \param  sc file descriptor della socket
 *   \param msg struttura che contiene il messaggio da scrivere 
 *   
 *   \retval  n    il numero di caratteri inviati (se scrittura OK)
 *   \retval  SEOF se il peer ha chiuso la connessione 
 *                   (non ci sono piu' lettori sulla socket) 
 *   \retval -1   in tutti gl ialtri casi di errore (sets errno)
 *
 */
int 
sendMessage(int sc, message_t * msg) 
{
  /* il campo buffer di un messaggio riferisce una stringa o NULL

     il campo length di un messaggio rappresenta la lunghezza del
     messaggio e conta anche il carattere di fine stringa 
  */
  int n = -1;

  /* verifico correttezza dei parametri */
  if ( 0 > sc || NULL == msg) {
    errno = EINVAL;
    return -1;
  }

  /* scrivo il messaggio passato nel socket */
  if ( -1 == write(sc, &msg->type, sizeof(msg->type)) ) {
    _handle_writeEOF; 
  }

  if ( -1 == write(sc, &msg->length, sizeof(msg->length)) ) {
    _handle_writeEOF;
  }
    
  if ( 0 < msg->length
       && -1 == (n = write(sc, msg->buffer, msg->length)) ) {
    _handle_writeEOF;
  }
  
  
  /*
  printf(">>>write return = %d, msg->length = %d\n", n, msg->length);
  */
  return 0;
}

/** crea una connessione all socket del server. In caso di errore
 *   funzione tenta NTRIALCONN volte la connessione (a distanza di 1
 *   secondo l'una dall'altra) prima di ritornare errore.  
 *   \param path  nome del socket su cui il server accetta le connessioni
 *   
 *   \return 0 se la connessione ha successo
 *   \retval SNAMETOOLONG se il nome del socket eccede UNIX_PATH_MAX
 *   \retval -1 negli altri casi di errore (sets errno)
 *
 *  in caso di errore ripristina la situazione inziale: rimuove
 *  eventuali socket create e chiude eventuali file descriptor rimasti
 *  aperti
 */
int 
openConnection(char * path)
{
  int result = 0, i = 0, err = 0;
  struct sockaddr_un serverAddr;
  socklen_t addrLen = sizeof(struct sockaddr_un);
  struct timespec toSleep, remainToSleep;

  /* verifica correttezza parametri */
  if (NULL == path) {
    errno = EINVAL;
    return -1;
  }
  if (UNIX_PATH_MAX < strlen(path))
    return SNAMETOOLONG;

  /* crea un socket attivo */
  if (-1 == (result = (socket(AF_UNIX, SOCK_STREAM, 0))))
    return -1;

  bzero(&serverAddr, sizeof(struct sockaddr_un));
  serverAddr.sun_family = AF_UNIX;
  strncpy(serverAddr.sun_path, path, sizeof(serverAddr.sun_path));

  toSleep.tv_nsec = 0; toSleep.tv_sec = 1;
  remainToSleep.tv_nsec = 0; remainToSleep.tv_sec = 0;

  /* prova a connetterti al server */
  while (NTRIALCONN > i  &&
	 -1 == (err = connect(result, (struct sockaddr *) &serverAddr, 
			      addrLen)))
  {
    if (-1 == nanosleep(&toSleep, &remainToSleep)) {
      close(result);
      return -1;
    }

    i++;
    fprintf(stderr, "prova numero %d\n", i); /* <-------------------- DBG -------------------- */
  } 
  
  /* la connessione non e' stata stabilita nei NTRIALCONN tentativi */
  if (-1 == err) {
    close(result);
    return -1;
  }
  
  return result;
}

/*
  while (NTRIALCONN > i  &&
	 -1 == (err = connect(result, (struct sockaddr *) &serverAddr, 
			      addrLen)))
  {
    do
    {
      if (-1 == nanosleep(&toSleep, &remainToSleep)) {
	if (EINTR == errno) {
	  toSleep.tv_nsec = remainToSleep.tv_nsec;
	  toSleep.tv_sec = remainToSleep.tv_sec;
	} else {
	  close(result);
	  return -1;
        }
      }

    } while(remainToSleep.tv_sec);

    i++;
    printf("prova numero %d\n", i);
  } 
  
*/
