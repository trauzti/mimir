/* $begin sbufc */
#include "csapp.h"
#include "sbuf.h"

/* Create an empty, bounded, shared FIFO buffer with n slots */
/* $begin sbuf_init */
void sbuf_init(sbuf_t *sp, int n)
{
	sp->buf = (unsigned int *)Calloc(n, sizeof(int)*3); 
	sp->n = n;					   /* Buffer holds max of n items */
	sp->front = sp->rear = 0;		/* Empty buffer iff front == rear */
	Sem_init(&sp->mutex, 0, 1);	  /* Binary semaphore for locking */
	Sem_init(&sp->slots, 0, n);	  /* Initially, buf has n empty slots */
	Sem_init(&sp->items, 0, 0);	  /* Initially, buf has zero data items */
}
/* $end sbuf_init */

/* Clean up buffer sp */
/* $begin sbuf_deinit */
void sbuf_deinit(sbuf_t *sp)
{
	Free(sp->buf);
}
/* $end sbuf_deinit */

/* Insert keyhash onto the rear of shared buffer sp */
/* $begin sbuf_insert */
void sbuf_insert(sbuf_t *sp, unsigned int type, unsigned int keyhash, unsigned int clsid)
{
	P(&sp->slots);						  /* Wait for available slot */
	P(&sp->mutex);						  /* Lock the buffer */
	sp->buf[(++sp->rear)%(sp->n)] = type;   /* Insert the type */
	sp->buf[(++sp->rear)%(sp->n)] = keyhash;   /* Insert the keyhash */
	sp->buf[(++sp->rear)%(sp->n)] = clsid;   /* Insert the clsid */
	V(&sp->mutex);						  /* Unlock the buffer */
	V(&sp->items);						  /* Announce available keyhash */
}
/* $end sbuf_insert */

/* Remove and return the first keyhash from buffer sp */
/* $begin sbuf_remove */
void sbuf_remove(sbuf_t *sp, unsigned int *rettype, unsigned int *retkeyhash, unsigned int *retclsid)
{
	P(&sp->items);						  /* Wait for available keyhash */
	P(&sp->mutex);						  /* Lock the buffer */
	*rettype    = sp->buf[(++sp->front)%(sp->n)];  /* Remove the type */
	*retkeyhash = sp->buf[(++sp->front)%(sp->n)];  /* Remove the keyhash */
	*retclsid   = sp->buf[(++sp->front)%(sp->n)];  /* Remove the keyhash */
	V(&sp->mutex);						  /* Unlock the buffer */
	V(&sp->slots);						  /* Announce available slot */
	return;
}
/* $end sbuf_remove */
/* $end sbufc */

