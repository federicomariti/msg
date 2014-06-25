#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include "../src/blockingList.h"


int compare_int(void *a, void *b) {
    int *_a, *_b;
    _a = (int *) a;
    _b = (int *) b;
    return ((*_a) - (*_b));
}
int compare_string(void *a, void *b) {
    char *_a, *_b;
    _a = (char *) a;
    _b = (char *) b;
    return strcmp(_a,_b);
}
/* funzione di copia di un intero */
void * copy_int(void *a) {
  int * _a;

  if ( ( _a = malloc(sizeof(int) ) ) == NULL ) return NULL;

  *_a = * (int * ) a;

  return (void *) _a;
}
/* funzione di copia di una stringa */
void * copy_string(void * a) {
  char * _a;

  if ( ( _a = strdup(( char * ) a ) ) == NULL ) return NULL;

  return (void *) _a;
}




void *
task1(void * arg)
{
  struct timespec tosleep;
  int err = (int) ((void **) arg) [0];

  tosleep.tv_nsec = 0;
  tosleep.tv_sec = (int) ((void **) arg ) [1];

  printf("START errno = %d\n", errno);

  errno = err;

  printf("START2 errno = %d\n", errno);

  nanosleep(&tosleep, NULL);

  perror("asd");
  printf("EXIT errno = %d\n", errno);

  return (void *) 0;
}



pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *
task2a(void * arg)
{
  printf("start task 2a\n");

  pthread_mutex_lock(&lock);

  do {

    printf("wait\n");
    pthread_cond_wait(&cond, &lock);

  } while (EINTR != errno);

  pthread_mutex_unlock(&lock);

  printf("end task 2a\n");


  return (void *) 0;
}

void *
task2b(void * arg)
{
  char buffer[8];
  printf("start task 2b\nwait\n");
  
  read(0, buffer, 8);

  printf("end task 2b\n");
}

void *
task2c(void * arg)
{
  char buffer[8];
  int bytesRead = 0;
  printf("start task 2c\nwait\n");
  
  while ( bytesRead = read(0, buffer, 8) 
	  &&  EINTR == errno )
    printf("task 2c: wait\n");

  if (bytesRead < 0)
    perror("task2c: read");

  printf("end task 2c\n");
}


/* LISTA CONDIVISA */
blockingList_t * list = NULL;

void *
task3(void * arg)
{
  printf("start task 3\n");

  do {

    printf("pop (wait)\n");
    blockingList_pop(list, NULL);

  } while (EINTR != errno);

  printf("end task 3\n");
  

  return (void *) 0;
}



void *
task6(void * arg)
{
  char * popped = NULL;
  
  sigset_t filtered;
  
  sigfillset(&filtered);
  pthread_sigmask(SIG_SETMASK, &filtered, NULL);  

  printf("task6: eseguo pop sulla lista condivisa\n");
  blockingList_pop(list, (void **)&popped);

  return NULL;
}

void *
task6_(void * arg)
{
  printf("task6: eseguo wait sulla var. condizione condivisa\n");
  pthread_mutex_lock(&lock);
  pthread_cond_wait(&cond, &lock);
  pthread_mutex_unlock(&lock);

  return NULL;
}





#define N 30

int
prova1(void)
{
  pthread_t thr[N];
  int arg[N][2];
  int i = 0;

  errno = 123;
  
  for (i=0; i<N; i++) {
    arg[i][0] = i;
    arg[i][1] = i * 2;
    pthread_create(thr+i, NULL, task1, arg+i);
  }

  for (i=0; i<N; i++)
    pthread_join(thr[i], NULL);

  printf("main: errno = %d\n", errno);
  
  return 0;
}

void
sa_ignoreIt(int sig)
{
  write(1, "asd\n", 4);
  ;
}

/* prova sulle system call interrompibili: 
 * che comportamento ha pthread_cond_wait ???
 *
 */
int
prova2(void)
{
  pthread_t thr[N];
  struct timespec tosleep;
  int i = 0;
  struct sigaction ignoreSignal_sa;
  sigset_t allSignal;
  char todo = 0;
  void * (*task) (void *);

  tosleep.tv_nsec = 0;
  tosleep.tv_sec = 2;

  sigfillset(&allSignal);

  bzero(&ignoreSignal_sa, sizeof(ignoreSignal_sa));
  ignoreSignal_sa.sa_handler = sa_ignoreIt;
  ignoreSignal_sa.sa_mask = allSignal;

  switch (todo) {
  case 0:
    /* attesa su pthread_cond_wait 
     *
     * mac: la s.c. e' interrotta dal segnale, 
     *      ritorna 0 e setta errno = EINTR
     *
     * lnx: la s.c. viene riavviata all'arrivo del segnale
     *
     */
    task = task2a;
    break;

  case 1:
    /* attesa su pthread_cond wait, come caso precedente
     *
     */
    ignoreSignal_sa.sa_flags = SA_RESTART;
    task = task2a;
    break;

  case 2:
    /* attesa su read, la s.c. viene interrotta, ritona -1 e setta
     * errno = EINTR
     *
     */
    task = task2b;
    break;

  case 3:
    /* attesa su read, la s.c. vine riavviata
     *
     */
    ignoreSignal_sa.sa_flags = SA_RESTART;
    task = task2b;
    break;

  case 4:
    /* attesa su read con gestione esplicita del riavvio
     *
     */
    task = task2c;
  }
  
  sigaction(SIGINT, &ignoreSignal_sa, NULL);

  printf("main: creo thread\n");

  for (i=0; i<N; i++)
    if (0 != pthread_create(thr+i, NULL, task, NULL)) {
      perror("prova2");
      return -1;
    }

  nanosleep(&tosleep, NULL);

  printf("main: killo thread\n");

  for (i=0; i<N; i++)
    pthread_kill(thr[i], SIGINT);

  for (i=0; i<N; i++)
    pthread_join(thr[i], NULL);
  
  return 0;
}


int
prova3(void)
{
  pthread_t thr[N];
  struct timespec tosleep;
  int i = 0;
  struct sigaction ignoreSignal_sa;
  sigset_t allSignal;

  sigfillset(&allSignal);

  bzero(&ignoreSignal_sa, sizeof(ignoreSignal_sa));
  ignoreSignal_sa.sa_handler = sa_ignoreIt;
  ignoreSignal_sa.sa_mask = allSignal;  

  sigaction(SIGINT, &ignoreSignal_sa, NULL);

  tosleep.tv_nsec = 0;
  tosleep.tv_sec = 2;

  list = blockingList_new(0, prova3, prova3);

  for (i=0; i<N; i++)
    if (0 != pthread_create(thr+i, NULL, task3, NULL)) {
      perror("prova3");
      return -1;
    }

  nanosleep(&tosleep, NULL);

  for (i=0; i<N; i++)
    pthread_kill(thr[i], SIGINT);


  /* pthread_cond_signal(&cond); */
  
  
  for (i=0; i<N; i++)
    pthread_join(thr[i], NULL);
  
  return 0;
}

void 
prova4_cleanup(void * arg)
{
  int * integer = (int *) arg;

  printf("cleanup: intero = %d\n", *integer);
}

int 
prova4(void)
{
  int integer = 123;

  pthread_cleanup_push(prova4_cleanup, &integer);

  printf("prova4: intero = %d\n", integer);

  pthread_exit(0);

  pthread_cleanup_pop(0);

  return 0;
}


int
prova5(void)
{
  int i = 10;
  
  while (i) {
    switch (i) {
    case 10: printf("%d\n", i); break;
    case 5: i = 0; continue;
    default: printf("asd ");
    }
    i--;
  }

  return 0;
}


int
prova6(void)
{
  struct sigaction sa;
  pthread_t t;
  struct timespec tosleep;

  tosleep.tv_nsec = 0;
  tosleep.tv_sec = 1;

  sigaction(SIGINT, NULL, &sa);  
  sa.sa_handler = sa_ignoreIt;
  sigaction(SIGINT, &sa, NULL);  

  printf("prova6: creo la lista condivisa\n");
  list = blockingList_new(0, copy_string, compare_string);

  printf("prova6: avvio l'altro thread\n");
  pthread_create(&t, NULL, task6_, NULL);

  if (EINTR == nanosleep(&tosleep, NULL)) {
    printf("prova6: interrotto\n");
  }

  printf("prova6: ho cancellato l'altro thread\n");
  pthread_cancel(t);
  printf("prova6: aspetto la terminazione dell'altro thread\n");
  pthread_join(t, NULL);

  return 0;
}

int
main() 
{
  return prova6();
}
