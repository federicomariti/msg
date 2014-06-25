/**
 * \file genList.c
 * \author Mariti Federico
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <errno.h>
#include <stdlib.h>
#include "genList.h"

extern int errno;

/**
   crea una lista generica

    \param compare funzione usata per confrontare due chiavi 
    \param copyk funzione usata per copiare una chiave
    \param copyp funzione usata per copiare un payload

    \retval NULL in caso di errori con \c errno impostata opportunamente
    \retval p (p!=NULL) puntatore alla nuova lista
*/
list_t * new_List(int (*compare) (void *, void *),   \
		  void * (*copyk) (void *),	     \
		  void * (*copyp) (void *)) {
  list_t * result = NULL;

  /* se almeno una funzione non e' definita ritorno lista non creata */
  if (NULL == compare || NULL == copyk || NULL == copyp) {
    errno = EINVAL;
    return NULL;
  }

  /* allocazione in memoria heap dello spazio necessario per una
     struttura list_t */
  if (NULL == (result = malloc(sizeof(list_t)))) {
    errno = ENOMEM; 
    return NULL; /* out of memory */
  }

  /* inizializzazione dei campi della nuova struttura allocata */
  result->head = NULL;
  result->compare = compare;
  result->copyk = copyk;
  result->copyp = copyp;

  /* ritorno la struttura inizializzata */
  return result;
}

/**
 distrugge una lista deallocando tutta la memoria occupata

    \param pt puntatore al puntatore della lista da distruggere

    nota: mette a NULL il puntatore \c *t
*/
void free_List (list_t ** pt) {
  elem_t * toRmv = NULL;

  /* controllo del parametro. per procedere nella deallocazione i
     puntatori alla lista non devono essere nulli */
  if (NULL == pt || NULL == *pt) {
    errno = EFAULT;
    return ; /* parametro non significativo */
  }

  /* deallocazione di tutti gli elementi che formano la lista puntata
     da pt */
  while (NULL != (toRmv = (*pt)->head)) {
    (*pt)->head = toRmv->next;
    free(toRmv->key);
    free(toRmv->payload);
    free(toRmv);
  }

  /* deallocazione della struttura della lista puntata da pt */
  free(*pt);  

  (*pt) = NULL;
}

/**
 inserisce una nuova coppia (key, payload) in testa alla lista, 
 sia key che payload devono essere copiate nel nuovo elemento della lista.
 Nella lista \b non sono permesse chiavi replicate

    \param t puntatore alla lista
    \param key la chiave
    \param payload l'informazione

    \retval -1 se si sono verificati errori (errno settato opportunamente)
    \retval 0 se l'inserimento \`e andato a buon fine
*/
int add_ListElement(list_t * t, void * key, void * payload) {
  elem_t * newElem_pelem = NULL;

  /* controllo valore dei parametri */
  if (NULL == t || NULL == key || payload == NULL) {
    errno = EINVAL;
    return -1; /* parametri non inizializzati */
  }

  /* cerca la nuova chiave nella lista */
  if (NULL != (find_ListElement(t, key))) {
    errno = EINVAL;
    return -1; /* chiave gia' presente nella lista */
  }

  /* allocazione di memoria heap per un elemento della lista generica */
  if (NULL == (newElem_pelem = malloc(sizeof(elem_t)))) {
    errno = ENOMEM;
    return -1; /* memoria non disponibile */  
  }

  /* inizilizza il nuovo elemento: copia il valore puntato da key e da
   * payload ed imposta come prossimo elemento la testa della lista
   * precedente.  
   *
   * nota: se la funzione copia chiave/payload della lista t ritorna
   * un valore nullo, allora viene gestita la memoria (deallocazione
   * delle nuove strutture create) e ritornato errore
   */
  if (NULL == (newElem_pelem->key = t->copyk(key))) {
    errno = ENOMEM;
    free(newElem_pelem);
    return -1; /* errore nella copia della chaive */
  }
  if (NULL == (newElem_pelem->payload = t->copyp(payload))) {
    errno = ENOMEM;
    free(newElem_pelem->key);
    free(newElem_pelem);
    return -1; /* errore nella copia del payload */
  }
  newElem_pelem->next = t->head;

  t->head = newElem_pelem;

  return 0;  
}
  
/**
 elimina l'elemento di chiave (key) deallocando la memoria

    \param t puntatore alla lista
    \param key la chiave

    \retval -1 se si sono verificati errori (errno settato opportunamente)
    \retval 0 se l'esecuzione e' stata corretta
*/
int remove_ListElement(list_t * t,void * key) {
  elem_t * toRmv_pelem = NULL;
  elem_t * itr_pelem = NULL;

  /* controllo del valore dei parametri */
  if (NULL == t || NULL == key) {
    errno = EINVAL;
    return -1; /* parametri con valori non significativi */
  }
  
  /* controllo lista vuota */
  if (NULL == t->head) {
    return 0; /* lista vuota
	       * (l'elemento di chiave key non esiste, terminazione corretta) */
  }

  /* controllo se la chiave da cancellare e' in testa alla lista */
  if (0 == (t->compare(t->head->key, key))) {
    toRmv_pelem = t->head;
    t->head = toRmv_pelem->next;

    free(toRmv_pelem->key);
    free(toRmv_pelem->payload);
    free(toRmv_pelem);
  } else {
    /* controllo se la chiave da cancellare e' nella lista, nel caso,
     * l'elemento corrispondente viene rimosso dalla lista */
    itr_pelem = t->head;
    while (NULL != itr_pelem->next) {
      if (0 == t->compare((itr_pelem->next)->key, key)) {
	toRmv_pelem = itr_pelem->next;
	itr_pelem->next = toRmv_pelem->next;
	free(toRmv_pelem->key);
	free(toRmv_pelem->payload);
	free(toRmv_pelem);
	break;
      } else 
	itr_pelem = itr_pelem->next;
    }
  }
  return 0;
}
  
/**
 cerca l'elemento di chiave (key)

    \param t puntatore alla lista
    \param key la chiave

    \retval NULL se l'elemento non c'e', errno settato opportunamente in caso si verifichino errori
    \retval p puntatore all'elemento trovato (puntatore interno alla lista non alloca memoria)
*/
elem_t * find_ListElement(list_t * t,void * key) {
  elem_t * itr_pelem = NULL; 

  /* controllo il valore dei parametri */
  if (NULL == t || NULL == key) {
    errno = EINVAL;
    return NULL; /* valore dei parametri non significativo */
  }

  /* controllo lista non vuota */
  if (NULL == (itr_pelem = t->head)) {
    errno = 0;
    return NULL; /* la lista t non ha elementi */
  }

  /* scorro tutta la lista fino a quando non trovo l'elemento cercato */
  do 
    if (0 == t->compare(itr_pelem->key, key))
      return itr_pelem; /* chiave trovata */
  while (NULL != (itr_pelem = itr_pelem->next));

  errno = 0;
  return NULL; /* chiave non trovata */
}
