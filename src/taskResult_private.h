/**
 *  \file taskResult_private.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef __TASKRESULT_PRIVATE_H__
#define __TASKRESULT_PRIVATE_H__

#include <pthread.h>

struct taskResult_t {
  void *           result;	/** riferimento al risultato */
  char             status;	/** stato di terminazione del task */
  pthread_mutex_t  mtx;		/** mutex  per l'accesso  esclusivo al
				    risultato ed allo stato */
  pthread_cond_t   monitor;	/** monitor  per  bloccarsi in  attesa
				    della produzione del risultato */
  
};


#endif /* __TASKRESULT_PRIVATE_H__ */
