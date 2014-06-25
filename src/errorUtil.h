/**
 *  \file errorUtil.h
 *  \author Federico Mariti
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.  
 */
#ifndef __ERRORUTIL_H__
#define __ERRORUTIL_H__

/* ====== debug util ==================================================== */

#define _print_dbg_pre				\
  printf("--- DBG: ");				\
  fflush(stdout);

#define _print_dbg_post				\
  fflush(stdout);				\
  printf(" ---\n");

#define _print_dbg(printfCmd)			\
  _print_dbg_pre;				\
  printfCmd;					\
  _print_dbg_post;

#define _print_dbgInfo(_infoMsg)		\
  printf("--- DBG: "_infoMsg" ---\n");

#define _print_dbgInfoIntInfo(_infoMsg1, _intValue, _infoMsg2)	\
  printf("--- DBG: "_infoMsg1"%d"_infoMsg2" ---\n", _intValue);	

#define _print_dbgInfoIntInfoInt(_infoMsg1, _intValue1,			\
			      _infoMsg2, _intValue2)			\
  printf("--- DBG: "_infoMsg1"%d"_infoMsg2"%d ---\n",			\
	 _intValue1, _intValue2);

#define _print_dbgString(_stringName)		\
  printf("--- DBG: %s ---\n", _stringName);

#define _print_dbgInfoString(_infoMsg, _stringName)	\
  printf("--- DBG: "_infoMsg"%s ---\n", _stringName);

#define _print_dbgStrInf(_stringName, _infoMsg)	\
  printf("--- DBG: %s: "_infoMsg" ---\n", _stringName);

#define _print_dbgStrInfStr(_stringName1, _infoMsg, _stringName2)	\
  printf("--- DBG: %s: "_infoMsg" %s ---\n", _stringName1, _stringName2);


/* ====================================================================== */

#define _handle_err(exp, errVal, tp_name, fun_name, todo)	\
{ int __err = 0;					\
  errno = 0;                                            \
  if ( errVal == (exp) ) {				\
    __err = errno;					\
    if (errno)						\
      perror(tp_name": "fun_name);			\
    todo;						\
  }                                                     \
}

#define _handle_err_str(exp, errVal, tp_name, fun_name, str_var, todo)	\
{ int __err = 0;					\
  errno = 0;                                            \
  if ( errVal == (exp) ) {				\
    __err = errno;					\
    if (errno)						\
       fprintf(stderr, tp_name": "fun_name": %s: %s\n", \
	       (char *) (str_var), strerror(errno));    \
    todo;						\
  }                                                     \
}

#define _handle_err_iide(exp, errVal, tp_name, tp_intide, fun_name, todo) \
{ int __err = 0;						       \
  errno = 0;							       \
  if ( errVal == (exp) ) {					       \
    __err = errno;						       \
    if (errno)							       \
      fprintf(stderr, tp_name"-%d: "fun_name": %s\n",		       \
	      tp_intide, strerror(errno));			       \
    todo;							       \
  }								       \
}

/* ====================================================================== */

#define _handle_meno1err(exp, tp_name, fun_name, todo)	\
  _handle_err(exp, -1, tp_name, fun_name, todo)

#define _handle_meno1err_str(exp, tp_name, fun_name, str_var, todo)	\
  _handle_err_str(exp, -1, tp_name, fun_name, str_var, todo)

#define _handle_meno1err_iide(exp, tp_name, tp_intide, fun_name, todo)	\
  _handle_err_iide(exp, -1, tp_name, tp_intide, fun_name, todo)

/* ---------------------------------------------------------------------- */

#define _handle_meno1err_ptexit(exp, tp_name, fun_name)	\
  _handle_meno1err(exp, tp_name, fun_name,		\
		   pthread_exit((void *) __err))

#define _handle_meno1err_ptexit_str(exp, tp_name, fun_name, str_var)	\
  _handle_meno1err_str(exp, tp_name, fun_name, str_var,		        \
		       pthread_exit((void *) __err))

#define _handle_meno1err_ptexit_iide(exp, tp_name, tp_intide, fun_name)	\
  _handle_meno1err_iide(exp, tp_name, tp_intide, fun_name,		\
			pthread_exit((void *) __err))

#define _handle_meno1err_exit(exp, tp_name, fun_name)		\
  _handle_meno1err(exp, tp_name,fun_name,			\
		   exit(-1))

#define _handle_meno1err_ptreturn(exp, tp_name, fun_name)	\
  _handle_meno1err(exp, tp_name, fun_name,			\
		   return (void *) __err)

#define _handle_meno1err_return(exp, tp_name, fun_name)	\
  _handle_meno1err(exp, tp_name, fun_name,		\
		    return __err)

/* ====================================================================== */

#define _handle_nullerr(exp, tp_name, fun_name, todo)	\
  _handle_err(exp, NULL, tp_name, fun_name, todo)

#define _handle_nullerr_iide(exp, tp_name, tp_intide, fun_name, todo)	\
  _handle_err_iide(exp, NULL, tp_name, tp_intide, fun_name, todo)

/* ---------------------------------------------------------------------- */

#define _handle_nullerr_ptexit(exp, tp_name, fun_name)	\
  _handle_nullerr(exp, tp_name, fun_name,		\
		   pthread_exit((void *) __err))

#define _handle_nullerr_ptexit_iide(exp, tp_name, tp_intide, fun_name)	\
  _handle_nullerr_iide(exp, tp_name, tp_intide, fun_name,		\
			pthread_exit((void *) __err))

#define _handle_nullerr_exit(exp, tp_name, fun_name)		\
  _handle_nullerr(exp, tp_name,fun_name,			\
		   exit(__err))

#define _handle_nullerr_ptreturn(exp, tp_name, fun_name)	\
  _handle_nullerr(exp, tp_name, fun_name,			\
		   return (void *) __err)

#define _handle_nullerr_return(exp, tp_name, fun_name)	\
  _handle_nullerr(exp, tp_name, fun_name,		\
		    return __err)

/* ====================================================================== */

#define _handle_ptherr(exp, thread_name, fun_name, todo)	\
{ int err = 0;                                          \
  if ( 0 != ( err = (exp) ) ) {				\
    errno = err;					\
    perror(thread_name": "fun_name);			\
    todo;						\
  }                                                     \
}

#define _handle_ptherr_str(exp, thread_name, fun_name, str_var, todo)	\
{ int err = 0;                                          \
  if ( 0 != ( err = (exp) ) ) {				\
    errno = err;					\
    fprintf(sterr, thread_name": "fun_name": %s: %s\n", \
	    (char *) (str_var), strerror(errno));       \
    todo;						\
  }                                                     \


#define _handle_ptherr_tiide(exp, thread_name,				\
				    thread_intide, fun_name, todo)	\
{ int err = 0;                                          \
  if ( 0 != ( err = (exp) ) ) {				\
    errno = err;                                        \
    fprintf(stderr, thread_name"-%d: "fun_name": %s\n",	\
	    thread_intide, strerror(err));		\
    todo;						\
  }                                                     \
}

#define _handle_ptherr_tname(exp, thread_str,	\
				    fun_name, todo)	\
{ int err = 0;                                          \
  if ( 0 != ( err = (exp) ) ) {				\
    errno = err;                                        \
    fprintf(stderr, "%s: "fun_name": %s\n",		\
	    thread_str, strerror(errno));		\
    todo;						\
  }                                                     \
}

/* ---------------------------------------------------------------------- */

#define _handle_ptherr_exit(exp, thread_name, fun_name)		\
  _handle_ptherr(exp, thread_name, fun_name,			\
		 pthread_exit((void *) err))

#define _handle_ptherr_exit_tiide(exp, thread_name,			\
				  thread_intide, fun_name)		\
  _handle_ptherr_tiide(exp, thread_name, thread_intide,			\
			fun_name, pthread_exit((void *) err))

#define _handle_ptherr_exit_tname(exp, thread_str, fun_name)	\
  _handle_ptherr_tname(exp, thread_str, fun_name,		\
		       pthread_exit((void *) err))


/* ---------------------------------------------------------------------- */

#define _handle_ptherr_return(exp, thread_name, fun_name)	\
  _handle_ptherr(exp, thread_name, fun_name,			\
		 return (void *) err;


#endif /* __ERRORUTIL_H__ */

