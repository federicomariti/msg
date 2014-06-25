#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

/*
 * test 0:  bloccare ricorsivamente un lock
 *   > default:     !DEAD-LOCK!
 *   > errorcheck:  return error EDEADLK
 *   > recursive:   ok
 *
 * test 1:  sbloccare un mtx che e' stato bloccato da un thread diverso
 *   > default:     ok
 *   > errorcheck:  return error EPERM
 *   > recursive:   return error EPERM
 *
 * test 2:  sbloccare un mtx che non e' bloccato
 *   > default:     ok
 *   > errorcheck:  return error EPERM
 *   > recursive:   return error EPERM
 *
 */

pthread_mutex_t mtx; /* = PTHREAD_MUTEX_INITIALIZER; */


#define mutex_init(mtxVar, mtxType)					\
  if ( 0 != (errno = pthread_mutexattr_init(&mattr)) ) {		\
    perror("Test0: pthread_mutexattr_init");				\
    return -1;								\
  }									\
  if ( 0 != (errno =							\
	     pthread_mutexattr_settype(&mattr,	mtxType)) ) {		\
    perror("Test0: pthread_mutex_errorcheck");				\
    return -1;								\
  }									\
  if ( 0 != (errno = pthread_mutex_init(&mtx, &mattr)) ) {		\
    perror("Test0: pthread_mutex_init");				\
    return -1;								\
  }									
  

int test0(int mtx_type) {
  pthread_t thr;
  pthread_mutexattr_t mattr;

  mutex_init(mtx, mtx_type);

  if ( 0 != (errno = pthread_mutex_lock(&mtx)) ) {
    perror("Test0: pthread_mutex_lock");
    return -1;
  }
  if ( 0 != (errno = pthread_mutex_lock(&mtx)) ) {
    perror("Test0: pthread_mutex_lock");
    return -1;
  }

  return 0;
}


void * task(void * arg) {
  
  if ( 0 != (errno = pthread_mutex_unlock(&mtx)) ) {
    perror("Test0: pthread_mutex_lock");
    return (void *) -1;
  }

  return NULL;
}

int test1(int mtx_type) {
  pthread_t thr;
  pthread_mutexattr_t mattr;

  mutex_init(mtx, mtx_type);

  if ( 0 != (errno = pthread_mutex_lock(&mtx)) ) {
    perror("Test0: pthread_mutex_lock");
    return -1;
  }

  if ( 0 != (errno = pthread_create(&thr, NULL, task, NULL)) ) {
    perror("test0: pthread_create");
    return -1;
  }
  if ( 0 != (errno = pthread_join(thr, NULL)) ) {
    perror("test0: pthread_join");
    return -1;
  }
  
  return 0;
}

int test2(int mtx_type) {
  pthread_mutexattr_t mattr;

  mutex_init(mtx, mtx_type);

  if ( 0 != (errno = pthread_mutex_unlock(&mtx)) ) {
    perror("Test0: pthread_mutex_lock");
    return -1;
  }

  return 0;
}

#include <unistd.h>
#include <limits.h>

int main(int argc, char ** argv) {

  int i = 0;
  const int default_mt = 0;

# if defined (__MACH__)
  const int errorcheck_mt = PTHREAD_MUTEX_ERRORCHECK;
  const int recursive_mt = PTHREAD_MUTEX_RECURSIVE;
#elif defined (__linux)
  const int errorcheck_mt = PTHREAD_MUTEX_ERRORCHECK_NP;
  const int recursive_mt = PTHREAD_MUTEX_RECURSIVE_NP;
# endif

  printf("_XOPEN_VERSION = %d\n", _XOPEN_VERSION);



  if (argc == 2) i = *argv[1] - '0';

  switch (i) {
  case 0: 
    printf("test0 con default type\n"); 
    i = test0(default_mt); 
    break;
  case 1: 
    printf("test0 con errorcheck type\n"); 
    i = test0(errorcheck_mt); 
    break;
  case 2:
    printf("test0 con recursive type\n"); 
    i = test0(recursive_mt); 
    break;

  case 3: 
    printf("test1 con default type\n"); 
    i = test1(default_mt); 
    break;
  case 4: 
    printf("test1 con errorcheck type\n"); 
    i = test1(errorcheck_mt); 
    break;
  case 5:
    printf("test1 con recursive type\n"); 
    i = test1(recursive_mt); 
    break;

  case 6: 
    printf("test2 con default type\n"); 
    i = test2(default_mt); 
    break;
  case 7: 
    printf("test2 con errorcheck type\n"); 
    i = test2(errorcheck_mt); 
    break;
  case 8:
    printf("test2 con recursive type\n"); 
    i = test2(recursive_mt); 
    break;

  default:
    i = 0;
  }

  printf("---\n");
  if ( i ) printf("Test failure\n");
  else printf("Test success\n");

  return i;
}
