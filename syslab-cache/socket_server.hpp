#ifndef SOCKET_SERVER_HPP
#define SOCKET_SERVER_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <event.h>
#include <signal.h>
#include <inttypes.h>

#include "cache_algorithm_base.hpp"
#include "common.hpp"


extern "C" {
#include "readqueue.h"
}

/* Port to listen on. */
#define SERVER_PORT 5555
/* Connection backlog (# of backlogged connections to accept). */
#define CONNECTION_BACKLOG 8
/* Socket read and write timeouts, in seconds. */
#define SOCKET_READ_TIMEOUT_SECONDS 100
#define SOCKET_WRITE_TIMEOUT_SECONDS 100

#define NUM_NETWORK_READER 8

/* Behaves similarly to fprintf(stderr, ...), but adds file, line, and function
 information. */
#define errorOut(...) {\
	fprintf(stderr, "%s:%d: %s():\t", __FILE__, __LINE__, __FUNCTION__);\
	fprintf(stderr, __VA_ARGS__);\
}

typedef struct client {
	/* The client's socket. */
	int fd;

	/* The event_base for this client. */
	struct event_base *evbase;

	/* The bufferedevent for this client. */
	struct bufferevent *buf_ev;

	/* The output buffer for this client. */
	struct evbuffer *output_buffer;

	/* Here you can add your own application-specific attributes which
	 * are connection-specific. */
} client_t;

void listen_for_requests(programoptions p, cache_algorithm_base *_alg);

#endif

