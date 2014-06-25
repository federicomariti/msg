/**
 *  \file threadPool_task_private.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef __THREADPOOL_TASK_PRIVATE_H__
#define __THREADPOOL_TASK_PRIVATE_H__

/** struttura  che  rappresenta  un  task sottomesso  al  thread  pool
 *  vengono mantenuti:
 *
 *  (function) il riferimento alla funzione che descrive il task,
 *
 *  (arg)  il riferimento  alla  base degli  argomenti della  funzione
 *  precedente,
 *
 *  (result) il riferimento alla  struttura che memorizza il risultato
 *  pendente dell'esecuzione del task, e
 *
 *  (returnErrorValue)  il  valore   di  ritorno  che  identifica  una
 *  esecuzione non corretta del task
 *
 *  nota: function e returnErrorValue sono sempre significativi, arg e
 *  result possono essere indefiniti se hanno valore NULL
 */
struct threadPool_task_t {
  void *      (* function) (void *);
  void *         arg; 		   /** can be null */
  taskResult_t * result;  	   /** can be null */
  void *         returnErrorValue; 

};


#define _threadPool_task_init(task, fun, arg, rslt, rtnErrVl)	\
  task.function = fun;						\
  task.arg = arg;						\
  task.result = rslt;						\
  task.rtnErrVl;



#endif /* __THREADPOOL_TASK_PRIVATE_H__ */
