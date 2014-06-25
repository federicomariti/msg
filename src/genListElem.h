/**
 *  \file genListElem.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */

#ifndef __GENLISTELEM__H
#define __GENLISTELEM__H

/** nodo della lista generica

 * Notare che sia la chiave (\b key) che l'informazione (\b payload) sono dei puntatori generici

 */
typedef struct elem {
  /** chiave */
  void * key;
  /** informazione */
  void * payload;
  /** puntatore elemento successivo */
  struct elem * next;
} elem_t; 

elem_t * new_genListElem(void * key, void * payload);

void * genListElem_getKey(void);

int genListElem_setKey(void * key);

void * genListElem_getPayload(void);

int genListElem_setPayload(void * payload);

#endif /* __GENLISTELEM__H */
