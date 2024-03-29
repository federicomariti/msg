/**
 *  \file genHash.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */

#ifndef __GENHASH__H
#define __GENHASH__H

#include "genList.h"

/**
   Tabella hash generica
 */
typedef struct {
  /** la tabella hash */
  list_t ** table;
  /** l'ampiezza della tabella */
  unsigned int size;
  /** la funzione per confrontare due chiavi */
  int (* compare) (void *, void *);
  /** la funzione per copiare una chiave */
  void * (* copyk) (void *);
  /** la funzione per copiare un payload*/
  void * (* copyp) (void *);
  /** la funzione hash*/
  unsigned int (* hash) (void *,unsigned int);
} hashTable_t;

/** FUNZIONI DA IMPLEMENTARE */

/** crea una tabella hash allocata dinamicamente
    \param size ampiezza della tabella
    \param compare funzione usata per confrontare due chiavi all'interno della tabella
    \param copyk funzione per copiare una chiave
    \param copyp funzione per copiare un payload
    \param hashfunction funzione hash (chiave,size della tabella)

    \retval NULL in caso di errori con \c errno impostata opportunamente
    \retval p (p!=NULL) puntatore alla nuova tabella allocata
*/
hashTable_t * new_hashTable (unsigned int size,  
			     int (* compare) (void *, void *), 
			     void* (* copyk) (void *),
			     void* (*copyp) (void*),
			     unsigned int (*hashfunction)(void*,unsigned int));


/** funzione hash per key di tipo int
   \param key valore chiave
   \param size ampiezza della hash table

   \retval index posizione nella tabella
*/
unsigned int hash_int (void * key, unsigned int size);

/** funzione hash per key di tipo string
   \param key valore chiave
   \param size ampiezza della hash table

   \retval index posizione nella tabella
*/
unsigned int hash_string (void * key, unsigned int size);
  


/** distrugge una tabella hash deallocando tutta la memoria occupata
    \param pt puntatore al puntatore della tabella da distruggere

    nota: mette a NULL il puntatore \c *pt
 */
void free_hashTable (hashTable_t ** pt);

/** inserisce una nuova coppia (key, payload) nella hash table (se non c'e' gia')

    \param t la tabella cui aggiungere
    \param key la chiave
    \param payload l'informazione

    \retval -1 se si sono verificati errori (errno settato opportunamente)
    \retval 0 se l'inserimento \`e andato a buon fine

    SP ricordarsi di controllare se (in caso di errore) si lascia la situazione consistente o si fa casino nella lista ....
 */
int add_hashElement(hashTable_t * t, void * key, void* payload );

#define define_hashTable_addElement(functionName, keyType, payloadType) \
  int									\
  functionName(hashTable_t * t, keyType key, payloadType payload)	\
  {									\
    return add_hashElement(t, key, payload);				\
  }

/** cerca una chiave nella tabella e restituisce il payload per quella chiave
   \param t la tabella in cui aggiungere
   \param key la chiave da cercare
  
   \retval NULL in caso di errore (errno != 0) 
   \retval p puntatore a una \b copia del payload (alloca memoria)
 */
void * find_hashElement(hashTable_t * t, void * key);

#define define_hashTable_findElement(functionName, keyType, payloadType) \
  payloadType								\
  functionName(hashTable_t * t, keyType key)				\
  {									\
    return (payloadType) find_hashElement(t, key);			\
  }  

/** se nella tabella esiste un elemento di chiave key allora la
 *  memoria occupata dal valore riferito dal campo payload di tale
 *  elemento viene deallocata e viene allocata la memoria per il nuovo
 *  valore, altrimenti la funzione ritorna errore
 *
 *  \param   t            riferimento alla tabella
 *
 *  \param   key          riferimento alla chiave a cui modificare il
 *                        payload 
 *
 *  \param   newpayload   riferimento al payload che andra a
 *                        sostituire quello precedente  
 *
 *  \param   oldpayload   se NULL la memoria occupata dal payload
 *                        precedente verra' liberata, altrimenti la
 *                        locazione riferita da puntera' al vecchio
 *                        valore di payload
 *
 *  \retval  0    tutto e' andato bene
 * 
 *  \retval  -1   si e' verificato un errore, variabile errno settata
 *
 *  \retval  -2   non esiste l'elemento di chiave key nella tabella t 
 */   
int setValue_hashTable(hashTable_t * t, void * key, 
		       void * newpayload, void ** oldpayload);

#define define_hashTable_setValue(functionName, keyType, payloadType)	\
  int									\
  functionName(hashTable_t * t, keyType key, payloadType newpayload,	\
	       payloadType * oldpayload)				\
  {									\
    return setValue_hashTable(t, (void *) key, (void *) newpayload,	\
			                      (void **) oldpayload);    \
  }



/** elimina l'elemento di chiave (key) deallocando la memoria

    \param t puntatore alla lista
    \param key la chiave


    \retval -1 se si sono verificati errori (errno settato opportunamente)
    \retval 0 se l'esecuzione e' stata corretta

 */
int remove_hashElement( hashTable_t * t,void * key );

#endif
