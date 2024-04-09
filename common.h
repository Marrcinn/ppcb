#pragma once
#include <cstdint>

constexpr bool DEBUG = true;
constexpr uint64_t MAX_DATA_SIZE = 10;

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

uint64_t generateSimpleRandomUint64();