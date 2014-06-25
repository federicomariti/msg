/**
 *  \file comsock.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef _COMSOCK_H
#define _COMSOCK_H

#include <string.h>

/* -= TIPI =- */

/* ---- v message v -------------------------------------------------- */

/** <H3>Messaggio</H3>
 * La struttura \c message_t rappresenta un messaggio 
 * - \c type rappresenta il tipo del messaggio
 * - \c length rappresenta la lunghezza in byte del campo buffer
 * - \c buffer e' il puntatore al messaggio (puo' essere NULL se length == 0)
 *
 * <HR>
 */
typedef struct {
    char type;           /** tipo del messaggio */
    unsigned int length; /** lunghezza in byte */
    char* buffer;        /** buffer messaggio */

} message_t; 

#define _createOkMessage(_msgVar)		\
  (_msgVar).type = MSG_OK;			\
  (_msgVar).length = 0;				\
  (_msgVar).buffer = NULL;

#define _createExitMessage(_msgVar) \
  (_msgVar).type = MSG_EXIT;          \
  (_msgVar).length = 0;               \
  (_msgVar).buffer = NULL;            

#define _createConnMessage(_msgVar, usrName) \
  (_msgVar).type = MSG_CONNECT;                \
  (_msgVar).length = strlen(usrName) + 1;	     \
  (_msgVar).buffer = usrName;

#define _createListRequest(_msgVar)      \
  (_msgVar).type = MSG_LIST;               \
  (_msgVar).length = 0;                    \
  (_msgVar).buffer = NULL; 

#define _createListMessage_(_msgVar, _listStr, _listStrLen)	\
  (_msgVar).type = MSG_LIST;                                      \
  (_msgVar).length = _listStrLen;                                 \
  (_msgVar).buffer = _listStr; 

#define _createListMessage(_msgVar, _listStr)			\
  (_msgVar).type = MSG_LIST;                                      \
  (_msgVar).length = strlen(_listStr) + 1				\
  (_msgVar).buffer = _listStr; 

#define _createToOneMessage(_msgVar, _text_or_stringVar)	\
  (_msgVar).type = MSG_TO_ONE;					\
  (_msgVar).length = strlen(_text_or_stringVar) + 1;		\
  (_msgVar).buffer = _text_or_stringVar;

#define _createToOneMessage_length(_msgVar, _text_or_stringVar, \
				   _messageLength)	        \
  (_msgVar).type = MSG_TO_ONE;					\
  (_msgVar).length = _messageLength;                     		\
  (_msgVar).buffer = _text_or_stringVar;

#define _createBcastMesssage(_msgVar, _text_or_stringVar)	\
  (_msgVar).type = MSG_BCAST;					\
  (_msgVar).length = strlen(_text_or_stringVar) + 1;		\
  (_msgVar).buffer = _text_or_stringVar;

#define _createBcastMesssage_length(_msgVar, _text_or_stringVar,\
				    _messageLength)        	\
  (_msgVar).type = MSG_BCAST;					\
  (_msgVar).length = _messageLength;                     		\
  (_msgVar).buffer = _text_or_stringVar;

#define _createErrorMessage(_msgVar, _text_or_stringVar)	\
  (_msgVar).type = MSG_ERROR;					\
  (_msgVar).length = strlen(_text_or_stringVar) + 1;		\
  (_msgVar).buffer = _text_or_stringVar;    


void * message_copy(void * a);
int message_compare(void * a, void * b); /* TODO */

/* ---- ^ message ^ -------------------------------------------------- */


/** lunghezza buffer indirizzo AF_UNIX */
#define UNIX_PATH_MAX    108

/** fine dello stream su socket, connessione chiusa dal peer */
#define SEOF -2
/** Error Socket Path Too Long (exceeding UNIX_PATH_MAX) */
#define SNAMETOOLONG -11 
/** numero di tentativi di connessione da parte del client */
#define  NTRIALCONN 5

/** tipi dei messaggi scambiati fra server e client */
/** richiesta di connessione utente */
#define MSG_CONNECT        'C'
/** errore */
#define MSG_ERROR          'E' 
/** OK */
#define MSG_OK             '0' 
/** NO */
#define MSG_NO             'N' 
/** messaggio a un singolo utente */
#define MSG_TO_ONE         'T' 
/** messaggio in broadcast */
#define MSG_BCAST          'B' 
/** lista utenti connessi */
#define MSG_LIST           'L' 
/** uscita */
#define MSG_EXIT           'X' 



/* -= FUNZIONI =- */
/** Crea una socket AF_UNIX
 *  \param  path pathname della socket
 *
 *  \retval s    il file descriptor della socket  (s>0)
 *  \retval SNAMETOOLONG se il nome del socket eccede UNIX_PATH_MAX
 *  \retval -1   in altri casi di errore (sets errno)
 *
 *  in caso di errore ripristina la situazione inziale: rimuove eventuali socket create e chiude eventuali file descriptor rimasti aperti
 */
int createServerChannel(char* path);

/** Chiude una socket
 *   \param s file descriptor della socket
 *
 *   \retval 0  se tutto ok, 
 *   \retval -1  se errore (sets errno)
 */
int closeSocket(int s);

/** accetta una connessione da parte di un client
 *  \param  s socket su cui ci mettiamo in attesa di accettare la connessione
 *
 *  \retval  c il descrittore della socket su cui siamo connessi
 *  \retval  -1 in casi di errore (sets errno)
 */
int acceptConnection(int s);

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
int receiveMessage(int sc, message_t * msg);

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
int sendMessage(int sc, message_t *msg);

/** crea una connessione all socket del server. In caso di errore
 *  funzione tenta NTRIALCONN volte la connessione (a distanza di 1
 *  secondo l'una dall'altra) prima di ritornare errore.
 *   \param  path  nome del socket su cui il server accetta le connessioni
 *   
 *   \return 0 se la connessione ha successo
 *   \retval SNAMETOOLONG se il nome del socket eccede UNIX_PATH_MAX
 *   \retval -1 negli altri casi di errore (sets errno)
 *
 *  in caso di errore ripristina la situazione inziale: rimuove eventuali socket create e chiude eventuali file descriptor rimasti aperti
 */
int openConnection(char* path);

#endif
