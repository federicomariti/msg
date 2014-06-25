/**
 *  \file msgservWriter.c
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include "msgserv.h"
#include "errorUtil.h"
#include "comsock.h"
#include "genHash.h"
#include "threadPool.h"
#include "blockingList.h"
#include "blockingList_itr.h"

extern char dbg;
extern FILE * logFile;
extern blockingList_t * logStack;

extern volatile sig_atomic_t leave;



/*
 *   TASK e FUNZIONE DI CLEAN UP DEL WRITER THREAD
 */

void
writerThread_cleanup(void * arg)
{
  char * nextLog = NULL;

  if (dbg) fprintf(stderr, "---DBG--- msgserv-writer: cleanup: start\n");

  blockingList_unlock(logStack); 

  while (!blockingList_isEmpty(logStack)) 
  {
    popLogMessage(logStack, &nextLog);
    if (dbg) fprintf(stderr, "---DBG--- msgserv-writer: cleanup: log = %s\n",
		     nextLog);
    fprintf(logFile, "%s\n", nextLog);
  }

  fclose(logFile);

  if (dbg) fprintf(stderr, "---DBG--- msgserv-writer: cleanup: end\n");
}

void *
writerTask(void * arg)
{
  sigset_t mySigmask_ss;
  char * nextLog = NULL;
  
  /*
   *   FILTRO TUTTI I SEGNALI 
   */
  sigfillset(&mySigmask_ss);
  _handle_ptherr_exit(  pthread_sigmask(SIG_SETMASK, 
					&mySigmask_ss, NULL),
			"writer", "pthread_sigmask"  );


  pthread_cleanup_push(writerThread_cleanup, NULL);


  if (dbg) fprintf(stderr, "---DBG--- msgserv-writer: start\n");

  /* la terminazione del thread corrente avviene con la cancellazione
     da parte del thread principale del processo. dopo la
     cancellazione, nella funzione di clean up, se la coda dei
     messaggi di log non e' vuota, vengono scritti i messaggi di log
     restanti */
  while (1)
  {
    if ( -1 == popLogMessage(logStack, &nextLog)) { /* C.P. */
      if (errno) perror("writerTask: popLogMessage");
      continue;
    }

      
    if (dbg) fprintf(stderr, "---DBG--- msgsev-writer: log = %s\n", nextLog);


    if ( 0 > fprintf(logFile, "%s\n", nextLog) ) {
      if (errno) perror("writerTask: fprintf");
      continue;
    }
    if ( -1 == fflush(logFile) ) 
      perror("writerTask: fflush");

    
    Free(nextLog);
  }

  pthread_cleanup_pop(0);
  fclose(logFile);
  blockingList_free(&logStack);

  if (dbg) fprintf(stderr, "---DBG--- msgserv-writer: terminated\n");

  return (void *) 0;
}
