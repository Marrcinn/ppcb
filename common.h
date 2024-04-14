#pragma once
#include <cstdint>

constexpr bool DEBUG = true;
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


void tcpSend(int socket_fd, void *data, uint32_t size);
void udpSend(int socket_fd, void *data, uint32_t size, struct sockaddr_in *server_addr);
void tcpReceive(int socket_fd, void *data, uint32_t size);
void udpReceive(int socket_fd, void *data, uint32_t size, struct sockaddr_in *server_addr);