/* $begin sbufc */
//#include "csapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include "sbuf.h"

void unix_error(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}


void Sem_init(sem_t *sem, int pshared, unsigned int value)
{
    if (sem_init(sem, pshared, value) < 0)
        unix_error("Sem_init error");
}

void P(sem_t *sem)
{
    if (sem_wait(sem) < 0)
        unix_error("P error");
}

void V(sem_t *sem)
{
    if (sem_post(sem) < 0)
        unix_error("V error");
}


/* Create an empty, bounded, shared FIFO buffer with n slots */
/* $begin sbuf_init */
void sbuf_init(sbuf_t *sp, int n)
{
	sp->buf = (unsigned int *)calloc(n, sizeof(int)*3); 
        if (sp->buf == NULL)
		unix_error ("malloc for sbuf calloc");
	sp->n = n;					   /* Buffer holds max of n items */
	sp->front = sp->rear = 0;		/* Empty buffer iff front == rear */
	Sem_init(&sp->rearmutex, 0, 1);	  /* Binary semaphore for locking */
	Sem_init(&sp->frontmutex, 0, 1);	  /* Binary semaphore for locking */
	Sem_init(&sp->slots, 0, n);	  /* Initially, buf has n empty slots */
	Sem_init(&sp->items, 0, 0);	  /* Initially, buf has zero data items */
}
/* $end sbuf_init */

/* Clean up buffer sp */
/* $begin sbuf_deinit */
void sbuf_deinit(sbuf_t *sp)
{
	free(sp->buf);
}
/* $end sbuf_deinit */

/* Insert keyhash onto the rear of shared buffer sp */
/* $begin sbuf_insert */
void sbuf_insert(sbuf_t *sp, unsigned int type, unsigned int keyhash, unsigned int clsid)
{
	/* Writer's time is valuable. If there is no slot, drop the request */
 	int val;
	sem_getvalue (&sp->slots, &val);
	if (val == 0) /* No slots available */
		return;
	P(&sp->slots);						  /* Wait for available slot */
	P(&sp->rearmutex);						  /* Lock the buffer */
	sp->buf[(++sp->rear)%(sp->n)] = type;   /* Insert the type */
	sp->buf[(++sp->rear)%(sp->n)] = keyhash;   /* Insert the keyhash */
	sp->buf[(++sp->rear)%(sp->n)] = clsid;   /* Insert the clsid */
	V(&sp->rearmutex);						  /* Unlock the buffer */
	V(&sp->items);						  /* Announce available keyhash */
}
/* $end sbuf_insert */

/* Remove and return the first keyhash from buffer sp */
/* $begin sbuf_remove */
void sbuf_remove(sbuf_t *sp, unsigned int *rettype, unsigned int *retkeyhash, unsigned int *retclsid)
{
	P(&sp->items);						  /* Wait for available keyhash */
	P(&sp->frontmutex);						  /* Lock the buffer */
	*rettype    = sp->buf[(++sp->front)%(sp->n)];  /* Remove the type */
	//*rettype = 1;
	*retkeyhash = sp->buf[(++sp->front)%(sp->n)];  /* Remove the keyhash */
	*retclsid   = sp->buf[(++sp->front)%(sp->n)];  /* Remove the keyhash */
	V(&sp->frontmutex);						  /* Unlock the buffer */
	V(&sp->slots);						  /* Announce available slot */
	return;
}
/* $end sbuf_remove */
/* $end sbufc */

