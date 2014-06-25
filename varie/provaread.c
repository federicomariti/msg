#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>

void *
task_reader(void * arg) 
{
  char buffer[32];

  read(0, buffer, 32);

  return  NULL;
}

int
main()
{
  pthread_t thr;
  struct timespec ts;
  
  printf("creo un thread che legge da stdin\n");
  pthread_create(&thr, NULL, task_reader, NULL);

  ts.tv_nsec = 0;
  ts.tv_sec = 2;

  printf("dormo 2 secondi\n");
  nanosleep(&ts, NULL);

  printf("cancello l'altro thread\n");
  pthread_cancel(thr);

  printf("aspetto la terminazione dell'altro thread\n");
  pthread_join(thr, NULL);

  return 0;
}
