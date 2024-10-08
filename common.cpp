#include "common.h"
#include <iostream>
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
#include <unistd.h>

// Probably not the best way to generate a random number,
// but it's good enough for what we need.
uint64_t generateSimpleRandomUint64() {
	srand(time(NULL) * getpid());
	uint64_t upper = rand();
	uint64_t lower = rand();
	return (upper << 32) | lower;
}

// Taken from source code from laboratories.
struct sockaddr_in get_tcp_server_address(char const *host, int port) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo *address_result;
	int errcode = getaddrinfo(host, NULL, &hints, &address_result);
	if (errcode != 0) {
		throw new std::runtime_error("ERROR: getaddrinfo failed\n");
	}

	struct sockaddr_in send_address;
	send_address.sin_family = AF_INET;   // IPv4
	send_address.sin_addr.s_addr =       // IP address
	    ((struct sockaddr_in *) (address_result->ai_addr))->sin_addr.s_addr;
	send_address.sin_port = htons(port);

	freeaddrinfo(address_result);

	return send_address;
}

// Taken from source code from laboratories.
struct sockaddr_in udp_get_server_address(char const *host, uint16_t port) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	struct addrinfo *address_result;
	int errcode = getaddrinfo(host, NULL, &hints, &address_result);
	if (errcode != 0) {
		throw std::runtime_error("ERROR: getaddrinfo failed\n");
	}
	struct sockaddr_in send_address;
	send_address.sin_family = AF_INET;   // IPv4
	send_address.sin_addr.s_addr =       // IP address
	    ((struct sockaddr_in *) (address_result->ai_addr))->sin_addr.s_addr;
	send_address.sin_port = htons(port);
	freeaddrinfo(address_result);
	return send_address;
}

void setSocketTimeout(int socket_fd, int seconds) {
	struct timeval timeout;
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
}

// Guarantees that the amount of data sent will be equal to
// the amount of data requested, otherwise throws error.
void tcpSend(int socket_fd, void *data, uint32_t size) {
	if constexpr (DEBUG) {
		std::cout << "Sending " << size << " bytes via tcpSend\n";
	}
	uint32_t bytes_sent = 0;
	while (bytes_sent < size) {
		int sent = send(socket_fd, (uint8_t*) data + bytes_sent, size - bytes_sent, 0);
		if (sent < 0) {
			throw  std::runtime_error("ERROR: send failed\n");
		}
		bytes_sent += sent;
	}
}

// Sends the whole data, possibly with more than one datagram.
void udpSend(int socket_fd, void *data, uint32_t size, struct sockaddr_in *server_addr) {
	if constexpr (DEBUG) {
		std::cout << "Sending " << size << " bytes via udpSend\n";
	}
	uint32_t bytes_sent = 0;
	while (bytes_sent < size) {
		int sent = sendto(socket_fd, (uint8_t*) data + bytes_sent, size - bytes_sent, 0,
		                      (struct sockaddr *) server_addr, sizeof(*server_addr));
		if (sent < 0) {
			throw  std::runtime_error("ERROR: sendto failed\n");
		}
		bytes_sent += sent;
	}
}

// Guarantees that the size of data received will be equal to size,
// otherwise throws an error.
int tcpReceive(int socket_fd, void *data, uint32_t size) {
	uint32_t bytes_received = 0;
	while (bytes_received < size) {
		int received = recv(socket_fd, (uint8_t*) data + bytes_received, size - bytes_received, 0);
		if (received < 0) {
			throw  std::runtime_error("ERROR: recv failed\n");
		}
		bytes_received += received;
	}
	return bytes_received;
}

// Receives data of size no more than parameter size. Can receive less data,
// if the nest datagram is smaller than number of bytes requested.
int udpReceive(int socket_fd, void *data, uint32_t size, struct sockaddr_in *server_addr) {
	uint32_t bytes_received = 0;
	socklen_t server_addr_len = sizeof(*server_addr);
	int received = recvfrom(socket_fd, (uint8_t*) data + bytes_received, size - bytes_received, 0,
	                           (struct sockaddr *) server_addr, &server_addr_len);
	if constexpr (DEBUG) {
		std::cout << "Received " << received << " bytes\n";
	}
	if (received < 0) {
		throw  std::runtime_error("ERROR: recvfrom failed\n");
	}
	return received;
}
