#include "common.h"

#include <stdexcept>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Probably not the best way to generate a random number,
// but it's good enough for this
uint64_t generateSimpleRandomUint64() {
	srand(time(NULL));
	uint64_t upper = rand();
	uint64_t lower = rand();
	return (upper << 32) | lower;
}

struct sockaddr_in get_tcp_server_address(char const *host, int port) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo *address_result;
	int errcode = getaddrinfo(host, NULL, &hints, &address_result);
	if (errcode != 0) {
		fprintf(stderr, "ERROR: getaddrinfo: %s\n", gai_strerror(errcode));
		exit(1);
	}

	struct sockaddr_in send_address;
	send_address.sin_family = AF_INET;   // IPv4
	send_address.sin_addr.s_addr =       // IP address
	    ((struct sockaddr_in *) (address_result->ai_addr))->sin_addr.s_addr;
	send_address.sin_port = htons(port); // port from the command line

	freeaddrinfo(address_result);

	return send_address;
}