/**
 *  \file myTest_threadPool.c
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include "threadPool.h"
#include "errorUtil.h"


/* variabile booleana per decidere se stampare le informazione di 
   debug o meno */
#ifdef DBG
static const char dbg = 1;
#else
static const char dbg = 0;
#endif

/* variabile booleana per decidere se stampare le informazioni dei 
   test o meno */
static char printInfo = 0;

#define _printDbg_tt(thread_name, text)				\
  if (dbg) printf("--- DBG:  "thread_name": "text"  ---\n");

#define _printInfo_tt(thread_name, textInfo)		\
  if (printInfo) printf(thread_name": "textInfo"\n");

#define _printInfo_tiidt(thread_name, thread_iide, textInfo)		\
  if (printInfo) printf(thread_name"-%d: "textInfo"\n", thread_iide);

#define _sleep(_timespec_var, _toSleep, _threadName)			\
  _timespec_var.tv_sec = (time_t)_toSleep;				\
  _handle_meno1err_ptexit(  nanosleep(&_timespec_var, NULL),		\
			    _threadName, "nanosleep"  );		

static const char * globalString = "questa e' una stringa con visibilita' globale";



/* ========================================================================
 * ==== vvv Some Tasks vvv ========================= ^^^ Test Util ^^^ ==== 
 * ========================================================================
 */

void *
idiotTask(void * arg)
{
  if (printInfo) 
    printf("thread-%d: lucy in the sky with diamonds\n", (int)pthread_self());

  return NULL;
}

/** blocca il thread corrente per (int)arg  secondi
 *
 *  \param   arg  e' un intero che rappresenta il numero di secondi
 *                da dormire
 *
 *  \retval  arg  tutto e' andato bene
 *
 *  \retval  (void *) -1  si e' verificato un errore, errno settato
 *
 */
void *
sleeperTask(void * arg)
{
  int secsToSleep = (int) arg;
  struct timespec toSleep;
  struct timespec rmTime;

  if (0 > secsToSleep) {
    errno = EINVAL;
    return (void *) -1;
  }

  if (printInfo) 
    printf("thread-%d: sleeperTask(%d) inizio\n", 
	   (int) pthread_self(), secsToSleep);

  toSleep.tv_nsec = 0;
  toSleep.tv_sec = secsToSleep;

  while (-1 == nanosleep(&toSleep, &rmTime))
    if(EINTR == errno) 
      toSleep = rmTime;
    else
      return (void *) -1;

  if (printInfo) 
    printf("thread-%d: sleeperTask(%d) terminato\n", 
	   (int) pthread_self(), secsToSleep);

  return arg;
}

/** task conto alla rovescia
 *
 *  \param  arg   intero che definisce il numero da contare
 *
 *  \retval (void *) 0  tutto e' andato bene
 *
 *  \retval (void *) -1 si e' verificato un errore, errno settato
 *
 */
void *
countdownTask(void * arg)
{
  int n = (int) arg;
  const int secs_toSleep = 1;
  struct timespec toSleep;


  toSleep.tv_nsec = 0;
  toSleep.tv_sec = secs_toSleep;
  
  for (; n>-1; n--) {
    printf("%d\n", n);
    if (-1 == nanosleep(&toSleep, NULL))
      return (void *) -1;
  }


  return (void *) 0;
}


/** task che calcola ricorsivamente il numero di fibionacci
 *  dell'ingresso
 *  
 *  \param  arg  intero su cui calcolare il numero di fibonacci
 *
 *  \retval  (void *) intero maggiore di 0 
 *
 */
void *
fiboTask(void * arg)
{
  if ((int) arg <  1)
    return (void *) 0;

  if ((int) arg == 1)
    return (void *) arg;

  return (void *) ((int)arg + (int)fiboTask( (void *) ((int)arg-1) ));
  
}




/* ========================================================================
 * ==== vvv Test Task vvv ========================= ^^^ Some Tasks ^^^ ==== 
 * ========================================================================
 */



/** creazione e distruzione 
 *  threadPool_init* e threadPool_shutdown*, threadPool_awaitTermination
 *
 */
int
test0(void)
{
  threadPool_t * tp_p = NULL;
  struct timespec toSleep;
  int i = 0;
  int secs[4][2] = { {0, 0}, {1, 0}, {0, 1}, {1, 1} };
  
  toSleep.tv_nsec = 1;
  toSleep.tv_sec = 1;

  for (i=0; i<4; i++) {
    _handle_nullerr_exit(  tp_p = threadPool_newFixed(10, 100),
			   "test0", "threadPool_newFixed"  );

    toSleep.tv_sec = secs[i][0];
    nanosleep(&toSleep, NULL);

    _handle_meno1err_exit(  threadPool_shutdownNow(tp_p),
			    "test0", "threadPool_shutdown"  );
    
    toSleep.tv_sec = secs[i][1];
    nanosleep(&toSleep, NULL);
    
    _handle_meno1err_exit(  threadPool_awaitTermination(tp_p),
			    "test0", "threadPool_awaitTermination"  );

  }

  return 0;
}


/** 
 * 
 */
int
test1(void)
{ 
  threadPool_t * tpool = NULL;
  struct timespec toSleep;
  int i = 0;
  const int n = 20;
  
  toSleep.tv_nsec = 0;
  toSleep.tv_sec = 1;

  _handle_nullerr_exit(  tpool = threadPool_newFixed(5, 5),
			 "test1", "threadPool_newFixed"  );
  for (i=0; i<n; i++)
    _handle_meno1err_exit(  threadPool_submit(tpool, 
					      idiotTask,
					      NULL,
					      NULL,
					      NULL),
			    "test1", "threadPool_submit"  );
  /*
  for(i=0; i<n; i++)
    _handle_meno1err_exit(  threadPool_submit(tpool, 
					      sleeperTask,
					      (void *)1,
					      NULL,
					      NULL),
			    "test1", "threadPool_submit"  );
  
  */
  nanosleep(&toSleep, NULL); 

  
  _handle_meno1err_exit(  threadPool_shutdown(tpool),
			  "test1", "threadPool_shutdown"  );

  _handle_meno1err_exit(  threadPool_awaitTermination(tpool),
			  "test1", "threadPool_awaitTermination"  );
  
  return 0;
}

/** 
 * 
 */
int
test2(void)
{

  
  return 0;
}

/** singolo submit di un task con ingresso e senza ritorno
 *  (countdownTask). contiguamente si richiede la terminazione e la si
 *  attende
 *
 */    
int
test3(void)
{
  threadPool_t * tpool = NULL;
  int arg_countdownTask = 5;
  
  
  _handle_nullerr_exit(  tpool = threadPool_newFixed(0, 10),
			  "test1", "threadPool_newFixed"  );

  _handle_meno1err_exit(  threadPool_submit(tpool,
					    countdownTask, 
					    &arg_countdownTask,
					    NULL,
					    (void *) -1),
			  "test1", "threadPool_submit"  );

  _handle_meno1err_exit(  threadPool_shutdown(tpool),
			  "test1", "threadPool_shutdown"  );

  _handle_meno1err_exit(  threadPool_awaitTermination(tpool),
			  "test1", "threadPool_awaitTermination"  );

  return 0;
}


/** piu' submit di un task con ingresso e senza ritorno
 * (countdownTask) . contiguamente si richiede la terminazione e la si
 * attende
 *
 */
int
test4(void)
{
  printf("TODO\n");
  return 0;
}


/** singolo submit di una attivita' con ingresso e valore di ritorno.
 *  sottometto il task (fiboTask) e stampo il risultato quando
 *  disponibile, quindi distruggo il thread pool
 *
 */
int
test5(void)
{
  printf("TODO\n");
  return 0;
}


/* ====================================================================== */

#define _doTest(_testFun, _testEs, _testInfo, _printFlag)		\
  {									\
    int __es = 0;							\
    if (_printFlag) printf(_testInfo"\n");				\
    _testEs = (__es = _testFun()) ? -1 : _testEs;			\
    if (_printFlag) {							\
      if (!__es) printf("   V  Completato\n");				\
      else printf("X  Fallito\n");					\
    }									\
  }

#define _pritnInfo printf("myTest_blockingList <t,p> [min] [max]\n"	\
			  "\tt    stampa il risultato complessivo dell'insieme dei test\n" \
			  "\tp    stampa il risultato per ogni test dell'insieme dei test\n" \
			  "\tmin  intero che identifica il primo test da eseguire (0-%d)\n" \
			  "\tmax  intero che identifica l'ultimo test da eseguire (0-%d)\n", \
			  _ntest-1, _ntest-1);
 

#define _ntest 5

int
main(int argc, char ** argv)
{
  int es = 0, i = 0, min = 0, max = 0;

  /* myTest_blockingList t x y  esegui i test da x a y
     myTest_blockingList t x    esegui il test x
     myTest_blockingList p x y  esegui i test da x a y con stampe
     myTest_blockingList p x    esegui il test x con stampe */

  if (argc < 1+1) {
    _pritnInfo;
    return 0;
  }

  if (*argv[1] == 't') printInfo = 0;
  else if (*argv[1] == 'p') printInfo = 1;
  else { _pritnInfo; return 0; }

  switch (argc) {
  case 1+1:
    min = 0; 
    max = _ntest - 1;
    break;
  case 2+1: 
    sscanf(argv[2], "%d", &max);
    min = max; break;
  case 3+1:
    min = *argv[2] - 48;
    max = *argv[3] - 48;
    break;
  default:
    _pritnInfo; return 0;
  }

  for (i=min; i<max+1; i++) {
    switch (i) {
    case 0:
      _doTest(test0, es, "Test creazione e cancellazione", printInfo);
      break;
    case 1:
      _doTest(test1, es, "Test singolo submit di un task senza ingressi e senza valori di ritorno, chiusura ed attesa della terminazione", printInfo);
      break;
    case 2:
      _doTest(test2, es, "Test piu' submit  di un task senza ingressi e senza valori di ritorno, chiusura ed attesa della terminazione", printInfo);
      break;
    case 3:
      _doTest(test3, es, "Test singolo submit di un task con ingressi e senza valore di ritorno, chiusura ed attesa della terminazione", printInfo);
      break;
    case 4:
      _doTest(test4, es, "Test piu' submit di un task con ingressi e senzavalore di ritorno, chiusura ed attesa della terminazione", printInfo);
      break;
    case 5:
      _doTest(test5, es, "Test singolo submit di un task con ingressi e con valore di ritorno, attesa del risultato e distruzione ", printInfo);
      break;
    }
  }

  if (!es) printf("\n\033[1;32mTest Set Superato!\n\033[0m");
  else printf("\n\033[1;31mTest Set Fallito\n\033[0m");

  return es;
}

