#pragma once
#include <cstdint>
#include <arpa/inet.h>

constexpr bool DEBUG = false;
constexpr uint64_t MAX_DATA_SIZE = 64000;
constexpr uint8_t MAX_WAIT = 5;
constexpr uint8_t MAX_RETRANSMITS = 5;

typedef struct __attribute__((__packed__)) {
	uint8_t type;
	uint64_t session_id;
	uint8_t protocol_id;
	uint64_t payload_length;
} CONN;

typedef struct __attribute__((__packed__)) {
	uint8_t type;
	uint64_t session_id;
} CONACC;

typedef struct __attribute__((__packed__)) {
	uint8_t type;
	uint64_t session_id;
	uint64_t packet_number;
	uint32_t data_length;
} DATA_HEADER;

typedef struct __attribute__((__packed__)){
	uint8_t type;
	uint64_t session_id;
	uint64_t packet_number;
} ACC;

typedef struct __attribute__((__packed__)) {
	uint8_t type;
	uint64_t session_id;
	uint64_t packet_number;
} RJT;

typedef struct __attribute__((__packed__)) {
	uint8_t type;
	uint64_t session_id;
} RCVD;

typedef struct __attribute__((__packed__)) {
	uint8_t type;
	uint64_t session_id;
} CONRJT;

struct sockaddr_in get_tcp_server_address(char const *host, int port);
struct sockaddr_in udp_get_server_address(char const *host, uint16_t port);

uint64_t generateSimpleRandomUint64();

void setSocketTimeout(int socket_fd, int seconds);

inline uint64_t ntohll(int64_t value) {
#if __BYTE_ORDER__ == __BIG_ENDIAN
	return value;
#else
	return (((int64_t) ntohl(value)) << 32) + ntohl(value >> 32);
#endif
}

inline uint64_t htonll(uint64_t n)
{
#if __BYTE_ORDER == __BIG_ENDIAN
	return n;
#else
	return (((uint64_t)htonl(n)) << 32) + htonl(n >> 32);
#endif
}

void tcpSend(int socket_fd, void *data, uint32_t size);
void udpSend(int socket_fd, void *data, uint32_t size, struct sockaddr_in *server_addr);
int tcpReceive(int socket_fd, void *data, uint32_t size);
int udpReceive(int socket_fd, void *data, uint32_t size, struct sockaddr_in *server_addr);