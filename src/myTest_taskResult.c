/**
 *  \file myTest_taskResult.c
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "taskResult.h"
#include "taskResult_private.h"
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



/* ======================================================================
 * ==== vvv Test Task vvv ======================== ^^^ Test Util ^^^ ====
 * ====================================================================== */




/** test creazione e distruzione 
 *
 */
int
test0(void)
{
  taskResult_t trslt;
  void * result;


  _handle_meno1err_exit(  taskResult_init(&trslt),
			  "test0", "taskResult_init"  );
  _handle_meno1err_exit(  taskResult_destroy(&trslt),
			  "test0", "taskResult_destroy"  );

  if (-1 != taskResult_isDone(&trslt)) {
    fprintf(stderr, "0\n"); return -1; }
  
  if (-1 != taskResult_getResult(&trslt, &result)) {
    fprintf(stderr, "1\n"); return -1; }

  if (-1 != taskResult_setResult(&trslt, (void *)123, TKRTS_DONE)) {
    fprintf(stderr, "2\n"); return -1; }

  return 0;
}



/** test vari delle funzioni con parametri non corretti 
 *
 */
int
test1(void)
{
  taskResult_t trslt;
  void * result = NULL;

  if (-1 != taskResult_init(NULL)) {
    fprintf(stderr, "0\n"); return -1; }

  _handle_meno1err_ptexit(  taskResult_init(&trslt),
			    "test1", "taskResult_init"  );


  if (-1 != taskResult_isDone(NULL)) { 
    if (printInfo) printf("1\n"); return -1; }



  if (-1 != taskResult_getResult(NULL, (void **)123)) { 
    if (printInfo) printf("2\n"); return -1; }
  
  if (-1 != taskResult_getResult(&trslt, NULL)) { 
    if (printInfo) printf("3\n"); return -1; }


  if (-1 != taskResult_setResult(NULL, (void *)123, TKRTS_DONE)) { 
    if (printInfo) printf("4\n"); return -1; }

  if (-1 != taskResult_setResult(&trslt, (void *)123, 123)) { 
    if (printInfo) printf("6\n"); return -1; }


  if (-1 != taskResult_destroy(NULL)) { 
    if (printInfo) printf("7\n"); return -1; }



  _handle_meno1err_ptexit(  taskResult_setResult(&trslt, NULL, TKRTS_DONE),
			    "test1", "taskResult_setResult"  );
  if (1 != taskResult_isDone(&trslt)) {
    if(printInfo) printf("8\n"); return -1; }
  _handle_meno1err_ptexit(  taskResult_getResult(&trslt, &result),
			    "test1", "taskResult_getResult"  );
  if (NULL != result) {
    if(printInfo) printf("9\n"); return -1; }



  _handle_meno1err_ptexit(  taskResult_setResult(&trslt, (void *) 123, TKRTS_DONE),
			    "test1", "taskResult_setResult2"  );
  if (1 != taskResult_isDone(&trslt)) {
    if(printInfo) printf("10\n"); return -1; }
  _handle_meno1err_ptexit(  taskResult_getResult(&trslt, &result),
			    "test1", "taskResult_getResult2"  );
  if (123 != (int)result) {
    if(printInfo) printf("11\n"); return -1; }
  

  return 0;
}


/** blocca il thread corrente blocca per (int)arg secondi
 *
 *  \param   arg   e' un intero che rappresenta il numero di secondi
 *                 da dormire
 *
 *  \retval  arg          tutto e' andato bene
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

  toSleep.tv_nsec = 0;
  toSleep.tv_sec = secsToSleep;

  while (-1 == nanosleep(&toSleep, &rmTime))
    if(EINTR == errno) 
      toSleep = rmTime;
    else
      return (void *) -1;

  return arg;
}


/** test produttore e consumatore di risultati corretti e erronei in un
 *  unico thread
 */
int
test2(void)
{
  void * (* taskFn) (void *) = sleeperTask;
  taskResult_t taskRlt_1, taskRlt_2;
  void * result_1, * result_2;
  char exitStatus_1, exitStatus_2;

  void * tmp_p;
  int    tmp_i;
  

  _handle_meno1err_exit(  taskResult_init(&taskRlt_1),
			  "test3", "taskResult_init1"  );

  _handle_meno1err_exit(  taskResult_init(&taskRlt_2),
			  "test3", "taskResult_init2"  );


  /* sono un produttore di risultati */

  if ( (void *) -1 != (result_1 = taskFn((void *) 1)) ) 
    exitStatus_1 = TKRTS_DONE;
  else {
    exitStatus_1 = TKRTS_ERROR;
    result_1 = (void *) errno;
  }

  _handle_meno1err_exit(  taskResult_setResult(&taskRlt_1, 
					       result_1,
					       exitStatus_1),
			  "test3", "taskResult_setResult1"  );


  if ( (void *) -1 != (result_2 = taskFn((void *) -123)) )
    exitStatus_2 = TKRTS_DONE;
  else {
    exitStatus_2 = TKRTS_ERROR;
    result_2 = (void *) errno;
  }

  _handle_meno1err_exit(  taskResult_setResult(&taskRlt_2,
					       result_2,
					       exitStatus_2),
			  "test3", "taskResult_setResult2"  );

  /* sono un consumatore di risultati */
  
  _handle_meno1err_exit(  tmp_i = taskResult_getResult(&taskRlt_1,
						       &tmp_p),
			  "test3", "taskResult_getResult"  );

  if (0 != tmp_i || tmp_p != result_1) {
    fprintf(stderr, "1\n"); return -1; }

  if (printInfo) printf("task result 1: prd = %d,  cns = %d  \n"
			"task compl normaly 1: prod = %d, cns = %d  \n\n",
			(int)result_1, (int)tmp_p, (int)exitStatus_1, tmp_i);
 


 
  _handle_meno1err_exit(  tmp_i = taskResult_getResult(&taskRlt_2,
						       &tmp_p),
			  "test3", "taskResult_getResult"  );
 
  if (-2 != tmp_i || (int) tmp_p != EINVAL) {
    fprintf(stderr, "2\n"); return -1; }


  if (printInfo) printf("task result 2: prd = %d,  cns = %d  \n"
			"task compl normaly 2: prod = %d, cns = %d  \n",
			(int)result_2, (int)tmp_p, (int)exitStatus_2, tmp_i);


  return 0;
}


void *
getterTask(void * arg)
{
  taskResult_t * trslt = (taskResult_t *) arg;
  void * result = NULL;


  if (printInfo) printf("getter: provo a ritirare il risultato\n");

  _handle_meno1err_exit(  taskResult_getResult(trslt, &result),
			   "tkrslt_getterTask", "taskResult_getResult"  );

  if (printInfo) printf("getter: risultato = >%s<\n", (char *)result);


  return (void *) 0;
}

void *
setterTask(void * arg)
{
  struct timespec toSleep;
  taskResult_t * trslt = (taskResult_t *) arg;

  if(printInfo) printf("setter: inizio elaborazione risultato (aspetto 1 secondo)\n");
  
  toSleep.tv_nsec = 0; toSleep.tv_sec = 1;
  _handle_meno1err_ptexit(  nanosleep(&toSleep, NULL),
			    "tkrslt_setterTask", "nanosleep"  );


  if (printInfo) printf("setter: settato il risultato = %s\n", globalString);
  _handle_meno1err_ptexit(  taskResult_setResult(trslt, 
						 (void *) globalString,
						 TKRTS_DONE),
			    "tkrslt_setterTask", "taskResult_setResult" );


  return (void *) 0; 
}


/** test isDone, getResult, setResult con un gruppo di thread che
 *   effettuano getResult ed uno solo che chiama setResult
 *
 */
int 
test3(void)
{
  taskResult_t trslt;
  pthread_t getter[10], setter;
  void * rvalGetter, * rvalSetter;
  int i = 0, es = 0; 


  _handle_meno1err_exit(  taskResult_init(&trslt),
			  "test3", "taskResult_init"  );

  /* creo i clienti */
  for (i=0; i<10; i++)
    _handle_meno1err_ptexit(  pthread_create(getter+i, NULL,
					     getterTask,
					     (void *) &trslt),
			      "test3", "pthread_create-for"  );

  /* creo il server*/
  _handle_meno1err_ptexit(  pthread_create(&setter, NULL,
					   setterTask, 
					   (void *) &trslt),
			    "test3", "pthread_create-1"  );

  /* aspetto il server */
  _handle_meno1err_ptexit(  pthread_join(setter, &rvalSetter),
			    "test3", "pthread_join-1"  );

  /* aspetto i clienti */
  for (i=0; i<10; i++) {
    _handle_meno1err_ptexit(  pthread_join(getter[i], &rvalGetter),
			      "test3", "pthread_join-for"  );
    es = rvalGetter ? -1 : es;
  }

  if (printInfo) printf("dispatcher: tutti i thread sono terminati,\n"
			"dispatcher: rimuovo la variabile taskResult_t\n");


  _handle_meno1err_ptexit(  taskResult_destroy(&trslt),
			    "test3", "taskResult_destroy"  );  

  es = rvalSetter ? -1 : es;

  
  return es;
}



/* ======================================================================
 * ==== vvv Main vvv ============================= ^^^ Test Task ^^^ ====
 * ====================================================================== */


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
 

#define _ntest 4

int
main(int argc, char ** argv)
{
  int es = 0, i = 0, min = 0, max = 3;

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
      _doTest(test0, es, "Test creazione e distruzione ", printInfo);
      break;
    case 1:
      _doTest(test1, es, "Test parametri non corretti", printInfo);
      break;
    case 2:
      _doTest(test2, es, "Test su taskResult, produttore e consumatore di risultati corrtti e erronei in un unico thread", printInfo);
      break;
    case 3:
      _doTest(test3, es, "Test ", printInfo);
      break;
    }
  }

  if (!es) printf("\n\033[1;32mTest Set Superato!\n\033[0m");
  else printf("\n\033[1;31mTest Set Fallito\n\033[0m");

  return es;
}
