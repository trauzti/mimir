#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

void log_header(int level, char *file, int line)
{
  time_t now;
  char time_str[1024];

  switch (level)
  {
  case INFOLEVEL:
    fprintf(stderr, ANSI_COLOR_GREEN"[ INFO:%15s:%4d] "ANSI_COLOR_RESET, file, line);
    break;
  case DEBUGLEVEL:
    fprintf(stderr, ANSI_COLOR_YELLOW"[DEBUG:%15s:%4d] "ANSI_COLOR_RESET, file, line);
    break;
  case ULTRADEBUGLEVEL:
    fprintf(stderr, ANSI_COLOR_BLUE"[ULTRA:%15s:%4d] "ANSI_COLOR_RESET, file, line);
    break;
  case ERRORLEVEL:
    fprintf(stderr, ANSI_COLOR_RED"[ERROR:%15s:%4d] "ANSI_COLOR_RESET, file, line);
    break;
  }

  now = time(NULL);
  strftime(time_str, 1024, "%a %d %b %Y %H:%M:%S ", localtime(&now));
  fprintf(stderr, "(pid=%d) %s  ::  ", getpid(), time_str);
}

void unix_error(char *msg)
{
  ERR("%s: %s\n", msg, strerror(errno));
  exit(0);
}


void posix_error(int code, char *msg)
{
  ERR("%s: %s\n", msg, strerror(code));
  exit(0);
}



void app_error(char *msg)
{
  ERR("%s\n", msg);
  exit(0);
}

/*
void *ReCalloc(void *ptr, size_t oldsize, size_t size)
{
  void *p;

  if ((p  = realloc(ptr, size)) == NULL)
    unix_error("Realloc error");
  if (size > oldsize)
    memset((char *p + oldsize, 0, size - oldsize);
  return p;
}
*/


void *Calloc(size_t nmemb, size_t size)
{
  void *p;

  if ((p = calloc(nmemb, size)) == NULL)
  {
    unix_error("Calloc error");
  }
  return p;
}


void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp,
                    void * (*routine)(void *), void *argp)
{
  int rc;

  if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
  {
    posix_error(rc, "Pthread_create error");
  }
}


void Pthread_detach(pthread_t tid)
{
  int rc;

  if ((rc = pthread_detach(tid)) != 0)
  {
    posix_error(rc, "Pthread_detach error");
  }
}
void Pthread_exit(void *retval)
{
  pthread_exit(retval);
}

pthread_t Pthread_self(void)
{
  return pthread_self();
}

void Pthread_mutex_lock(pthread_mutex_t *mutex)
{
  if ( pthread_mutex_lock(mutex) != 0)
  {
    app_error("pthread_mutex_lock");
  }
}

void pretty_print(item *p)
{
  DEB("item(key=%s, activity=%d)\n",
          ITEM_key(p), p->activity);
}
