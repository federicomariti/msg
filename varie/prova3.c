#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t monitor = PTHREAD_COND_INITIALIZER;

int cond = 0;

void
task_cleanup(void * arg) 
{
  printf("task: clean up\n");
  pthread_mutex_unlock(&lock);
}

void *
task(void * arg)
{
  pthread_cleanup_push(task_cleanup, NULL);

  pthread_mutex_lock(&lock);
  while (!cond) {
    printf("task: eseguo wait sulla var. condizione condivisa\n");
    pthread_cond_wait(&monitor, &lock);
  }
  pthread_mutex_unlock(&lock);

  pthread_cleanup_pop(0);

  return NULL;
}

#define _bufsz 8
int 
readLine(FILE * fl, char ** strref)
{
  int bytesRead = 0, bfsz = _bufsz, bfactlsz = 0, unused = 0;
  char leave = 0, * tmp = NULL;

  if (NULL == fl  || NULL == strref) {
    errno = EINVAL;
    return -1;
  }

  bfactlsz = bfsz;
  
  while (!leave) 
  {
    if( NULL == (tmp = realloc(*strref, bfactlsz)) ) {
      free(*strref); *strref = NULL;
      return -1; /* oom */
    }
    *strref = tmp; 
    bzero((*strref)+bfactlsz-bfsz, bfsz);
  
    fgets(*strref+bfactlsz-bfsz-unused, bfsz+unused, fl);

    tmp = *strref+bfactlsz-bfsz-unused;
    while (!leave  &&  tmp < *strref + bfactlsz)
    {
      if ('\n' == *tmp) {
	leave = 1;
      } else
	tmp++;
    }

    bfactlsz += bfsz;
    unused = 1;
  } 
  
  *tmp = '\0';
  unused = (*strref)+bfactlsz-bfsz - tmp-1;
  if ( NULL == (tmp = realloc(*strref, bfactlsz-bfsz -  unused)) ) {
    free(strref); *strref = NULL;
    return -1;
  }

  return 0;
}


int
main(void)
{
  pthread_t t;
  struct timespec tosleep;

  char bf[8];
  char * str = NULL;
  FILE * fl;

  fl = fopen("DATA/userlist", "r");
  
  readLine(fl, &str);

  printf("%d, >%s<\n", strlen(str), str);

  free(str);

  fclose(fl);

  return 0;

  tosleep.tv_nsec = 0;
  tosleep.tv_sec = 1;

  printf("main: avvio l'altro thread\n");
  pthread_create(&t, NULL, task, NULL);

  nanosleep(&tosleep, NULL);

  printf("main: ho cancellato l'altro thread\n");
  pthread_cancel(t);
  printf("main: aspetto la terminazione dell'altro thread\n");
  pthread_join(t, NULL);

  return 0;
}
