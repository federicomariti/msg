/**
 *  \file blockingList.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef __BLOCKINGLIST_H__
#define __BLOCKINGLIST_H__

/** Lista bloccante, si intende una lista i cui metodi sono eseguiti
 *  in mutua esclusione e sono implementati meccanismi bloccanti sulle
 *  condizioni "lista piena" e "lista vuota" nelle operazioni di
 *  inserzione e estrazione rispettivamente
 *
 *  interfaccia per una pila (lista LIFO)
 *    inserisci in testa:  blockingList_push(l, v)
 *    rimuovi in testa:    blockingList_pop(l, &v)
 *    esamina testa:       blockingList_peek(l), blockingList_pop(l, NULL) 
 *
 *  interfaccia per una coda (lsita FIFO)
 *    inserisci in coda:  blockingList_add(l, v)
 *    rimuovi in testa:   blockingList_poll(l), blockingList_pop(l, &v)
 *    esamina testa:      blockingList_peek(l), blockingList_pop(l, NULL) 
 *    
 */


struct blockingList_t;

typedef struct blockingList_t blockingList_t;

/** inizializza una lista generica bloccante gia' allocata 
 *
 *  \param  list       riferimento alla lista
 *
 *  \param  maxSize    massima dimensone della lista
 *
 *  \param  copy_fun   riferimento alla funzione di copia degli
 *                     elementi della lista 
 *
 *  \param  cmpr_fun   riferimento alla funzione di comparazione degli
 *                     elementi della lista
 *
 *  \retval   0        tutto e' andato bene
 *
 *  \retval  -1        si e' verificato un errore, errno settato
 *
 */
int blockingList_init(blockingList_t * list, int maxSize, 
		      void * (* copy_fun)(void *),
		      int (* cmpr_fun)(void *, void *));

/** alloca in memoria heap ed inizializza  una lista generica
 *  bloccante gia' allocata  
 *
 *  \param  maxSize    massima dimensone della lista
 *
 *  \param  copy_fun   riferimento alla funzione di copia degli
 *                     elementi della lista 
 *
 *  \param  cmpr_fun   riferimento alla funzione di comparazione degli
 *                     elementi della lista
 *
 *  \return            il riferimento alla lista appena creata
 *
 *  \retval  NULL      si e' verificato un errore, errno settato
 *
 */
blockingList_t * blockingList_new(int maxSize, 
				  void * (* copy_fun)(void *),
				  int (* cmpr_fun)(void *, void *));

/** libera la memoria occupata dalle strutture riferite dai campi
 *  della struttura lista
 *
 *  \param  list   riferimento alla lista da distruggere
 *
 *  \retval  0     tuto e' andato bene
 *
 *  \retval  -1    si e' verificato un errore
 *
 */
int blockingList_destroy(blockingList_t * list);

/** libera la memoria occupata dalle strutture riferite dai campi
 *  della struttura lista, e la struttura lista stessa
 *
 *  \param  list   riferimento al puntatore della lista
 *
 */
void blockingList_free(blockingList_t ** list);

/** prende il lock associato alla lista per l'accesso esclusivo alle
 *  funzionalita' della lista, fino alla successiva invocazione di
 *  blockingList_unlock
 *
 *  \param  list  riferimento alla lista
 *
 *  \retval  0    tutto e' andato bene
 *
 *  \retval  -1   si e' verificato un errore, errno settato
 *
 */
int blockingList_lock(blockingList_t * list);

/** rilascia il lock associato alla lista.
 *
 *  \param  list  riferimento alla lista
 *
 *  \retval  0    tutto e' andato bene
 *
 *  \retval  -1   si e' verificato un errore, errno settato
 *
 */
int blockingList_unlock(blockingList_t * list);

/** blocca il thread corrente sul monitor associato alla
 *  lista. la sveglia puo' essere causata da una invocazione esplicita
 *  di blockingList_notify(all) da parte di un altro thread, o il
 *  verificarsi di una condizione di sveglia implicita della lista
 *  ("lista vuota" e "lista piena")
 *
 *  \param  list  riferimento alla lista
 *
 *  \retval  0    tutto e' andato bene
 *
 *  \retval  -1   si e' verificato un errore, errno settato
 *
 */
int blockingList_wait(blockingList_t * list);

/** invoca la notifica sul monitor associato alla lista svegliando un
 *  solo thread in attesa
 *
 *  \param  list  riferimento alla lista
 *
 *  \retval  0    tutto e' andato bene
 *
 *  \retval  -1   si e' verificato un errore, errno settato
 *
 */
int blockingList_notify(blockingList_t * list);

/** invoca la notifica sul monitor associato alla lista svegliando
 *  tutti i thread in attesa
 *
 *  \param  list  riferimento alla lista
 *
 *  \retval  0    tutto e' andato bene
 *
 *  \retval  -1   si e' verificato un errore, errno settato
 *
 */
int blockingList_notifyAll(blockingList_t * list);

/** verifica se la lista e' vuota o meno, tale funzione non richiede
 *  l'accesso esclusivo alla lista
 *
 *  \param  list  riferimento alla lista
 *
 *  \retval  0    la lista non e' vuota
 *
 *  \retval  1    la lista e' vuota
 *
 *  \retval  -1   si e' verificato un errore, errno settato
 *
 */
int blockingList_isEmpty(blockingList_t * list);

/** inserisce in testa alla lista un nuovo elemento che riferisce la
 *  copia del valore puntato dal parametro valuep
 *
 *  se la lista contine il numero massimo degli elementi il thread
 *  corrente si blocca fin tanto che un elemento venga estratto o un
 *  segnale sia ricevuto dal thread stesso. Nel caso in cui venga
 *  ricevuto un segnale viene settatala variabile errno a EINTR e
 *  viene ritornato -1
 *
 *  puo' svegliare eventuali thread in attesa sulla rimozione di un
 *  elemento dalla lista
 *
 *  \param  list    riferimento alla lista
 *
 *  \param  valuep  riferimento al valore da copiare e riferire in
 *                  un nuovo elemento della lista
 *
 *  \retval  0    tutto e' andato bene
 *
 *  \retval  -1   si e' verificato un errore, errno settato
 *
 */
int blockingList_push(blockingList_t * list, void * valuep); /* SYNC */

#define define_blockingList_push(functionName, valueType)	\
  int								\
  functionName(blockingList_t * list, valueType valuep)		\
  {								\
    return blockingList_push(list, (void *) valuep);		\
  }

/** rimuove l'elemento in testa alla lista (l'ultimo elemento inserito
 *  se la lista e' usata come pila con le funzioni blockingList_push e
 *  blockingList_pop) il riferimento all'elemento e' salvato in
 *  *valuep se queto e' diverso da NULL, altrimenti viene deallocata
 *  la memoria occupata da tale valore.
 *
 *  se la lista e' vuota il thread che ha invocato tale funzione viene
 *  sopseso fin tanto che non sia disponibile un elemento o fin tanto
 *  che un segnale sia ricevuto dal thread stesso. Nel caso in cui
 *  venga ricevuto un segnale viene settata la variabile errno a EINTR
 *  e viene ritornato -1
 *
 *  puo' svegliare eventuali thread in attesa sull'inserzione di un
 *  elemento nella lista
 *
 *  \param  list    il riferimento alla lista
 *
 *  \param valuep   il riferimento ad una locazione di memoria che
 *                  conterra' il puntatore all'elemento rimosso
 *
 *  \retval 0   tutto e' andato bene
 *
 *  \retval -1  si e' verificato un errore, errno e' settato
 *
 *  nota bene: se  valuep e' NULL  il valore del  primo elemento della
 *             lista sara' deallocato  dalla memoria. Se invece valuep
 *             e' diverso da  NULL, il valore dell'elemento cancellato
 *             non e' deallocato dalla memoria, cio' deve essere fatto
 *             esplicitamente dall'utente con free(*valuep)!
 *
 */
int blockingList_pop(blockingList_t * list, void ** valuep); /* SYNC */

#define define_blockingList_pop(functionName, valueType)		\
  int									\
  functionName(blockingList_t * list, valueType valuep)			\
  {									\
    return blockingList_pop(list, (void **) valuep);			\
  }

/** si comporta come blockingList_pop
 *
 */
int blockingList_poll(blockingList_t * list, void ** valuep); /* SYNC */

#define define_blockingList_poll(functionName, valueType)		\
  int									\
  functionName(blockingList_t * list, valueType valuep)			\
  {									\
    return blockingList_poll(list, (void **) valuep);			\
  }

/** 
 *  rimuove l'elemento in coda alla lista (il primo inserito se per le
 *  inserzioni e' stata usata solo la funzione blockingList_add). se
 *  dest e' diverso da NULL il riferimento al valore di tale elemento
 *  e' salvato in *dest altrimenti viene deallocata la memoria
 *  occupata da tale valore.
 *
 *  se la lista e' vuota il thread che ha invocato tale funzione viene
 *  sopseso fin tanto che non sia disponibile un elemento o fin tanto
 *  che un segnale sia ricevuto dal thread stesso. Nel caso in cui
 *  venga ricevuto un segnale viene settata la variabile errno a EINTR
 *  e viene ritornato -1
 *  puo' svegliare eventuali thread in attesa sull'inserzione di un
 *  elemento nella lista
 *
 *  \param  list    il riferimento alla lista
 *
 *  \param valuep   il riferimento ad una locazione di memoria che
 *                  conterra' il puntatore all'elemento rimosso
 *
 *  \retval 0   tutto e' andato bene
 *
 *  \retval -1  si e' verificato un errore, errno e' settato
 *
 *  nota bene: se  valuep e' NULL il valore dell'ultimo elemento della
 *             lista sara' deallocato  dalla memoria. Se invece valuep
 *             e' diverso da  NULL, il valore dell'elemento cancellato
 *             non e' deallocato dalla memoria, cio' deve essere fatto
 *             esplicitamente dall'utente con free(*valuep)!
 *
 */
int blockingList_take(blockingList_t * list, void ** dest); /* SYNC */

#define define_blockingList_take(functionName, valueType)               \
  int                                                                   \
  functionName(blockingList_t * list, valueType dest)                   \
  {                                                                     \
     return blockingList_take(list, (void **) dest);                    \
  }                                                                     


/** inserisce in coda alla lista un nuovo elemento che riferisce la
 *  copia del valore puntato dal parametro valuep
 *
 *  se la lista contine il numero massimo degli elementi il thread
 *  corrente si blocca fin tanto che un elemento venga estratto o un
 *  segnale sia ricevuto dal thread stesso. Nel caso in cui venga
 *  ricevuto un segnale viene settata la variabile errno a EINTR e
 *  viene ritornato -1
 *
 *  puo' svegliare eventuali thread in attesa sulla rimozione di un
 *  elemento dalla lista
 *
 *  \param  list    riferimento alla lista
 *
 *  \param  valuep  riferimento al valore da copiare e riferire in
 *                  un nuovo elemento della lista
 *
 *  \retval  0    tutto e' andato bene
 *
 *  \retval  -1   si e' verificato un errore, errno settato
 *
 */
int blockingList_add(blockingList_t * list, void * valuep); /* SYNC */

#define define_blockingList_add(functionName, valueType)		\
  int									\
  functionName(blockingList_t * list, valueType valuep)			\
  {									\
    return blockingList_add(list, (void *) valuep);			\
  }


/** rimuove il  primo elemento nella  lista che ha valore  "uguale" al
 *  valore riferito da valuep.
 *  per  valore "uguale"  si intende  il ritorno  0 della  funzione di
 *  comparazione di list.
 *
 *  se la lista e' vuota il thread che ha invocato tale funzione viene
 *  sopseso fin tanto che non sia disponibile un elemento o fin tanto
 *  che un segnale non sia ricevuto dal thread stesso. Nel caso in cui
 *  venga ricevuto un segnale viene settatala variabile errno a EINTR
 *  e viene ritornato -1
 *
 *  puo' svegliare eventuali thread in attesa sull'inserzione di un
 *  elemento nella lista
 *
 *  \param   list    il riferimento della lista
 *
 *  \param   valuep  il valore dell'elemento della lista da rimuovere
 *
 *  \retval  0       tutto e' andato bene.
 *                   se il  valore e' stato  trovato, il corrispettivo
 *                   elemento della lista e' stato rimosso, altrimenti
 *                   la lista non viene modificata
 *
 *  \retval  -1      si e' verificato un errore, errno settato
 *
 */
int blockingList_remove(blockingList_t * list, void * valuep); /* SYNC */

#define define_blockingList_remove(functionName, valueType)		\
  int									\
  functionName(blockingList_t * list, valueType valuep)			\
  {									\
    return blockingList_remove(list, (void *) valuep);			\
  }


int blockingList_removeAll(blockingList_t * list, void * valuep); /* TODO */

#define define_blockingList_removeAll(functionName, valueType)		\
  int									\
  functionName(blockingList_t * list, valueType valuep)			\
  {									\
    return blockingList_removeAll(list, (void *) valuep);		\
  }

/** ricerca un elemnto nella lista, ritorna vero se esiste almeno una
 *  occorrenza nella lista
 *
 *  \param  list   il riferimento alla lista
 *
 *  \param  valuep  il riferimento all'elemento da ricercare
 *
 *  \retval 0  non esistono occorrenze dell'elemento nella lista
 *
 *  \retval 1  esiste almeno una occorrenza dell'elemento nella lista
 *
 *  \retval -1 errore, errno settato
 *
 */
int blockingList_contains(blockingList_t * list, void * valuep); /* SYNC */

#define define_blockingList_contains(functionName, valueType)		\
  int									\
  functionName(blockingList_t * list, valueType valuep)			\
  {									\
    return blockingList_contains(list, (void *) valuep);		\
  }

/** rimuove tutti gli elementi dalla lista 
 *
 *
 */
int blockingList_clear(blockingList_t * list); /* SYNC */

/** ritorna la dimensione (numero di elementi) attuale della lista
 *
 *
 */
int blockingList_getSize(blockingList_t * list);

/** rtorna la dimensione massima (numero di elementi) della lista
 *
 *
 */
int blockingList_getMaxSize(blockingList_t * list);

/** preleva senza rimuovere il valore dell'elemento in testa alla
 *  lista
 *
 *
 */
void * blockingList_peek(blockingList_t * list);

#endif /* __BLOCKINGLIST_H__ */
