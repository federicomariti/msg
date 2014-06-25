/**
 *  \file listElem.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef __LISTELEM_H__
#define __LISTELEM_H__

typedef struct listElem_t {
  struct listElem_t *  next;
  void *               value;

} listElem_t;

listElem_t * listElem_new(listElem_t * next, void * value, 
			  void * (* copy_fun) (void *)); /* TODO */

void listElem_free(listElem_t ** elemp); /* TODO */



#define _listElem_init(_elemp, _value, _next)	\
  _elemp->value = _value;			\
  _elemp->next = _next;

#define _listElem_new(_elemp, _value, _next)		\
  if ( NULL == (_elemp = malloc(sizeof(listElem_t))) )	\
    return -1;						\
  _listElem_init(_elemp, _value, _next)  

#define _listElem_free(_elemp)			\
  free(_elemp->value);				\
  free(_elemp);

#endif /* __LISTELEM_H__ */
