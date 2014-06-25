/**
 *  \file threadPool_task.c
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "threadPool_task.h"
#include "threadPool_task_private.h"



threadPool_task_t * 
threadPool_task_new(void * (* fun) (void *),
		    void * arg, 
		    taskResult_t * rslt, 
		    void * returnErrVal) 
{
  threadPool_task_t * result = NULL;

  if (NULL == fun) {
    errno = EINVAL;
    return NULL;
  }

  if ( NULL == ( result = (malloc(sizeof(threadPool_task_t))) ) )
    return NULL;

  result->function = fun;
  result->arg = arg;
  result->result = rslt;
  result->returnErrorValue = returnErrVal;

  return result;
}

void 
threadPool_task_free(threadPool_task_t ** task) 
{
  taskResult_free(&(*task)->result);
  free(*task);
  *task = NULL;
}

int 
threadPool_task_init(threadPool_task_t * task,
		     void * (* fun) (void *), void * arg, 
		     taskResult_t* rslt, void * rtnErrVl) 
{
  if (NULL == task || NULL == fun) {
    errno = EINVAL;
    return -1;
  }

  task->function = fun;
  task->arg = arg;
  task->result = rslt;
  task->returnErrorValue = rtnErrVl;
    
  return  0;
}



void * 
threadPool_task_copy(void * a) 
{
  threadPool_task_t * _a = NULL;

  if ( NULL == ( _a = malloc(sizeof(threadPool_task_t)) ) )
    return (void *) -1;


  _a->function = ((threadPool_task_t *) a)->function;
  _a->arg = ((threadPool_task_t *) a)->arg; /* ???? */
  _a->result = ((threadPool_task_t *) a)->result; /* ???? */
  _a->returnErrorValue = ((threadPool_task_t *) a)->returnErrorValue;
  

  return (void *) _a;
}

int 
threadPool_task_compare(void * a, void * b) 
{
  threadPool_task_t * _a = (threadPool_task_t *) a,
    * _b = (threadPool_task_t *) b;

  return _a->function == _b->function  &&
    _a->arg == _b->arg  &&	/* ???? */
    _a->result == _b->result   && /* ???? */
    _a->returnErrorValue == _b->returnErrorValue;
}
