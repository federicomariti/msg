#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>


void
task_cleanup(void * arg)
{
  printf("task_cleanup: !\n");
}

void *
task(void * arg)
{
  struct timespec ts;
  char buf[8];
  
  ts.tv_nsec = 0;
  ts.tv_sec = 2;

  pthread_cleanup_push(task_cleanup, NULL);

  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

  printf("task: dormo\n"); 

  nanosleep(&ts, NULL);

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

  /* pthread_testcancel(); */ /* <---- cp */

  read(0, buf, 8); /* <---- cp */

  pthread_cleanup_pop(0);

  printf("task: fine\n");

  return NULL;
}

int
main() 
{
  pthread_t thr;
  struct timespec ts;

  ts.tv_nsec = 0;
  ts.tv_sec = 1;

  pthread_create(&thr, NULL, task, NULL);

  nanosleep(&ts, NULL);

  printf("main: cancellato\n");

  pthread_cancel(thr);

  pthread_join(thr, NULL);

  printf("main: fine\n");

  return 0;
}
  
