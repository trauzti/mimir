#ifndef __SBUF_H__
#define __SBUF_H__

#include <pthread.h>
#include <semaphore.h>


/* $begin sbuft */
typedef struct {
    unsigned int *buf;          /* Buffer array */         
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t rearmutex;       /* Protects accesses to buf */
    sem_t frontmutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} sbuf_t;
/* $end sbuft */

extern void sbuf_init(sbuf_t *sp, int n);
extern void sbuf_deinit(sbuf_t *sp);
extern void sbuf_insert(sbuf_t *sp, unsigned int type, unsigned int item, unsigned int clsid);
extern void sbuf_remove(sbuf_t *sp, unsigned int *type, unsigned int *item, unsigned int *clsid);

#endif /* __SBUF_H__ */
