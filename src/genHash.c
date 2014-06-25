/**
 * \file genHash.c
 * \author Mariti Federico
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "genHash.h"

/**
   funzione hash per key di tipo int
   \param key valore chiave
   \param size ampiezza della hash table

   \retval index posizione nella tabella
*/
unsigned int
hash_int (void * key, unsigned int size) 
{
  int value = *(int *) key;
  /*
  unsigned int result = (value * 2654435761 ) % size; 
   2654435761 e' sezione aurea di 2^32 */

  unsigned int result = (value-(value/size)) % size; 
  return result;
  /* return (*(int *)key) % size; */
}

/**
   funzione hash per key di tipo string
   \param key valore chiave
   \param size ampiezza della hash table

   \retval index posizione nella tabella
*/
unsigned int
hash_string (void * key, unsigned int size) 
{
  unsigned int hash = 0;
  char * string_itr = key;

  while (*string_itr != '\0') {
    hash = ((hash * 31) + *string_itr) % size;
    string_itr++;
  }

  return hash;
}


/**
   crea una tabella hash allocata dinamicamente
    \param size ampiezza della tabella
    \param compare funzione usata per confrontare due chiavi all'interno della tabella
    \param copyk funzione per copiare una chiave
    \param copyp funzione per copiare un payload
    \param hashfunction funzione hash (chiave,size della tabella)

    \retval NULL in caso di errori con \c errno impostata opportunamente
    \retval p (p!=NULL) puntatore alla nuova tabella allocata
*/
hashTable_t * new_hashTable (unsigned int size,				\
			     int (* compare) (void *, void *),		\
			     void* (* copyk) (void *),			\
			     void* (*copyp) (void*),			\
			     unsigned int (*hashfunction)(void*,unsigned int)) {
  hashTable_t * result = NULL;

  /* controllo valore dei paramtri */
  if (size <= 0 || compare == NULL || copyk == NULL ||	
      copyp == NULL || hashfunction == NULL ) {
    errno = EINVAL; /* valore dei parametri non corretto */
    return NULL;
  }
  
  /* allocazione in memeoria heap della struttura hashTable_t */
  if (NULL == (result = malloc(sizeof(hashTable_t)))) {
    errno = ENOMEM; /* out of memory */
    return NULL;
  }

  /* inizializzazione del campo table dellla struttura hashTable_t ed
   * allocazione in memoria heap dell'array di puntatori a list_t.
   *
   * nota: la memoria allocata e' inizilizzata a zero (NULL) */
  if (NULL == (result->table = calloc(size, sizeof(list_t *)))) {
    free(result); /* deallocazione della struttura hashTable_t */
    errno = ENOMEM; /* memoria non disponibile */
    return NULL;
  }

  /* nota: l'allocazione/deallocazione delle struttre list_t della
   * tabella hash e' gestita dalle funzioni add e remove della hash
   * stessa */
  
  /* inizializzazione degli altri campi della struttura hashTable_t */
  result->size = size;
  result->compare = compare;
  result->copyk = copyk;
  result->copyp = copyp;
  result->hash = hashfunction;

  return result;
}

/**
   distrugge una tabella hash deallocando tutta la memoria occupata
    \param pt puntatore al puntatore della tabella da distruggere

    nota: mette a NULL il puntatore \c *pt
*/
void free_hashTable (hashTable_t ** pt) {
  list_t ** listItr_pp = NULL;
  int i = 0;

  /* controllo valore dell'argomento */
  if (NULL == pt) {
    errno = EINVAL;
    return ;
  }

  /* controllo valore del puntatore alla struttura hash riferita dal
   * paramentro ed il valore del puntatore alla tabella hash nella 
   * struttura */
  if (NULL == *pt || NULL == (*pt)->table) {
    errno = EFAULT;
    return ;
  }

  listItr_pp = (*pt)->table; 

  /* deallocazione delle liste */
  for (i=0; i<(*pt)->size; i++)
    free_List(listItr_pp+i);
  
  /* deallocazione della tabella */
  free((*pt)->table);

  /* deallocazione della struttura della tabella hash */
  free(*pt);
  
  *pt = NULL;
}

/**
   inserisce una nuova coppia (key, payload) nella hash table (se non c'e' gia')

    \param t la tabella cui aggiungere
    \param key la chiave
    \param payload l'informazione

    \retval -1 se si sono verificati errori (errno settato opportunamente)
    \retval 0 se l'inserimento \`e andato a buon fine

    SP ricordarsi di controllare se (in caso di errore) si lascia la
    situazione consistente o si fa casino nella lista ....
*/
int add_hashElement(hashTable_t * t,void * key, void* payload ) {
  int index = 0;

  /* controllo valore dei parametri */
  if (!t || !key || !payload || !(t->table)) {
    errno = EINVAL;
    return -1;
  }

  /* indice della tabella hash per la chiave da inserire */
  index = t->hash(key, t->size);

  /* verifica che sia allocatanella, nella tabella hash, la lista per
   * l'indice hash della chiave da inserire */
  if (NULL == *(t->table+index)) {
    /* crea la lista per l'indice index della tablella */
    if (NULL == (*(t->table+index) = new_List(t->compare, t->copyk, t->copyp)))
      return -1; /* impossibile creare la lista in index di hash table
		  * (memoria non disponibile) */
  } 

  /* aggiungi un nuovo elemento alla lista (key, payload) */
  errno = 0;
  if (-1 == add_ListElement(*(t->table+index), key, payload)) {
    /* dato che NULL != { *(t->table+index), key, payload }
     * allora e' impossibile aggiungere un nuovo elemento 
     * - perche' la memoria non e' disponibile
     * - perche' esiste gia' un elemento con chiave key
     * in entrambi i casi errno viene opportunamente impostato
     * da add_ListElement */

    if (NULL == (*(t->table+index))->head) {/* <================================================== ACCESSO AI CAMPI DELLA STRUTTURA LIST_T */
      /* la lista dell'indice corrispondente alla nuova chiave e'
       * vuota, cio' implica che e' stata allocata da questa
       * insersione e deve essere deallocata, in quanto l'insersione
       * e' fallita.
       *
       * nota: la free_List imposta a NULL il valore del puntatore
       * riferito dal puntatore passato per argomento, quindi
       * *(t->table+index) assumera' NULL, che significa lista non
       * presente per l'indice index della tabella hash. */

      free_List(t->table+index);
    }

    return -1;
  }
  
  return 0;
}

/**
   cerca una chiave nella tabella e restituisce il payload per quella chiave
   \param t la tabella in cui aggiungere
   \param key la chiave da cercare
  
   \retval NULL in caso di errore (errno != 0) 
   \retval p puntatore a una \b copia del payload (alloca memoria)
*/
void * find_hashElement(hashTable_t * t, void * key) {
  int key_index = 0;
  elem_t * elem_p = NULL;
  void * result = NULL;

  /* controllo valore degli argomenti */
  if (NULL == t || NULL == key) {
    errno = EINVAL;
    return NULL; /* argomenti non corretti */
  }

  /* controllo valore putatore alla tabella hash */
  if (NULL == t->table) {
    errno = EFAULT;
    return NULL; /* hash table non corretta */
  }

  /* indice della tabella hash per la chiave da inserire */
  key_index = t->hash(key, t->size);

  /* controlla che ci sia almeno un elemento in key_index della
   * tabella has */
  if (NULL == *(t->table+key_index)) {
    errno = 0; /* nessun errore */
    return NULL; /* chiave non trovata */
  }
  
  /* cerca l'elemento di chiave key nella lista in index della tabella hash */
  if (NULL == (elem_p = find_ListElement(*(t->table+key_index), key))) {
    return NULL; /* non esiste l'elemento di chiave key nella tabella */
  }
  
  /* crea una copia del campo payload dell'elemento trovato e ritorna
   * il riferimento a questo al chiamante */
  if (NULL == (result = t->copyp(elem_p->payload))) {
    errno = ENOMEM;
    return NULL; /* memoria non disponibile */
  }

  return result;
}

/**
   elimina l'elemento di chiave (key) deallocando la memoria

    \param t puntatore alla lista
    \param key la chiave


    \retval -1 se si sono verificati errori (errno settato opportunamente)
    \retval 0 se l'esecuzione e' stata corretta
*/
int remove_hashElement(hashTable_t * t, void * key) {
  int key_index = 0;

  /* controllo valore argomenti */
  if (NULL == t || NULL == key) {
    errno = EINVAL;
    return -1;
  }

  /* conrollo valore del puntatore alla tabella hash */
  if (NULL == t->table) {
    errno = EFAULT;
    return -1;
  }

  /* calcola l'indice per la chiave da rimuovere dalla tabella hash */
  key_index = t->hash(key, t->size);

  /* se la lista dell'indice corrispondente alla chiave da rimuovere
   * non  e' vuota, allora viene invocata la remove sulla questa lista
   * per la chiave key */
  if (NULL == *(t->table+key_index))
    return 0; /* l'indice della tabella hash corrispondente alla
	       * chiave e' privo di elementi */
  else
    if (-1 == remove_ListElement(*(t->table+key_index), key))
      return -1; /* non dovrebbe mai verificarsi */    

  /* l'elemento di chiave key e' stato rimosso se era presente nella
   * hash table 
   *
   * nota: se la lista in index della tabella hash e' vuota occorre
   * deallocarola */
  if (NULL == (*(t->table+key_index))->head) /* <================================================== ACCESSO AI CAMPI DELLA STRUTTURA LIST_T */
    free_List(t->table+key_index); /* dealloca la lista in key_index
				    * diventata vuota dopo la remove */
    
  return 0;
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
int 
setValue_hashTable(hashTable_t * t, void * key, 
		       void * newpayload, void ** oldpayload)
{
  int key_index = 0;
  elem_t * elem_p = NULL;
  void * newValue_p = NULL;

  if ( NULL == t || NULL == key || NULL == newpayload ) {
    errno = EINVAL;
    return -1;
  }


  /* controllo valore putatore alla tabella hash */
  if (NULL == t->table) {
    errno = EFAULT;
    return -1; /* hash table non corretta */
  }

  /* indice della tabella hash per la chiave da inserire */
  key_index = t->hash(key, t->size);

  /* controlla che ci sia almeno un elemento in key_index della
   * tabella has */
  if (NULL == *(t->table+key_index)) {
    errno = 0; /* nessun errore */
    return -2; /* chiave non trovata */
  }
  
  /* cerca l'elemento di chiave key nella lista in index della tabella hash */
  if (NULL == (elem_p = find_ListElement(*(t->table+key_index), key))) {
    return -2; /* non esiste l'elemento di chiave key nella tabella */
  }
  
  if ( NULL == (newValue_p = t->copyp(newpayload)) ) {
    errno = ENOMEM;
    return -1;
  }

  if (NULL == oldpayload)
    free(elem_p->payload);
  else
    *oldpayload = elem_p->payload;

  elem_p->payload = newValue_p;

  return 0;
}
