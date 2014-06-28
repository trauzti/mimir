#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include <time.h>

#include "memcached.h"

/* Enable/disable debugging */
// libcuckoo uses DEBUG
#define __DEBUG 1  
//#define ULTRADEBUG 0

#define TOLERANCE 1.0e-9

#define DEB_FLAG 1 

#define INFOLEVEL 1
#define ERRORLEVEL 2
#define DEBUGLEVEL 3
#define ULTRADEBUGLEVEL 4

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#ifndef DEB_FLAG 

#define flushandreset {  fprintf(stderr, ANSI_COLOR_RESET); fflush(stderr); }

#define INF(FMT, ...) { log_header(INFOLEVEL, __FILE__, __LINE__); fprintf(stderr, FMT, ##__VA_ARGS__); flushandreset }
#define ERR(FMT, ...) { log_header(ERRORLEVEL, __FILE__, __LINE__); fprintf(stderr, FMT, ##__VA_ARGS__); flushandreset }

#ifdef __DEBUG
#define DEB(FMT, ...) { log_header(DEBUGLEVEL, __FILE__, __LINE__); fprintf(stderr, FMT, ##__VA_ARGS__); flushandreset }
#else
#define DEB(FMT, ...) { do {} while (0); }
#endif

#ifdef ULTRADEBUG
#define UDB(FMT, ...) { log_header(ULTRADEBUGLEVEL, __FILE__, __LINE__); fprintf(stderr, FMT, ##__VA_ARGS__); flushandreset }
#else
#define UDB(FMT, ...) { do {} while (0); }
#endif

#else
#undef __DEBUG

#define INF(FMT, ...) {do {} while(0);}
#define DEB(FMT, ...) {do {} while(0);}
#define UDB(FMT, ...) {do {} while(0);}
#define ERR(FMT, ...) {do {} while(0);}

#endif



void log_header(int level, char *file, int line);
void unix_error(char *msg);
void posix_error(int code, char *msg);
void app_error(char *msg);

void *ReCalloc(void *ptr, size_t oldsize, size_t size);
void *Calloc(size_t nmemb, size_t size);

pthread_t Pthread_self(void);
void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp, void * (*routine)(void *), void *argp);
void Pthread_detach(pthread_t tid);
void Pthread_exit(void *retval);
void Pthread_mutex_lock(pthread_mutex_t *mutex);

void pretty_print(item *p);
#endif /* COMMON_H */
