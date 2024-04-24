#pragma once
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <endian.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"

class Client {
private:
	std::string protocol;
	uint8_t protocol_id;
	sockaddr_in server_addr;
	int socket_fd;
	uint64_t session_id;
	uint64_t cur_packet_number = 0;
	std::shared_ptr<char> data;
	uint8_t buffer[64400];
	uint64_t payload_length;

void send(void *data, uint32_t size) {
	if (this->protocol == "tcp") {
		tcpSend(this->socket_fd, data, size);
	} else {
		udpSend(this->socket_fd, data, size, &this->server_addr);
	}
}

void receive(void *data, uint32_t size) {
	if (this->protocol == "tcp") {
		tcpReceive(this->socket_fd, data, size);
	} else {
		udpReceive(this->socket_fd, data, size, &this->server_addr);
	}
}

public:
	Client(std::string protocol, std::string address, int port) {
		this->protocol = protocol;
		this->session_id = generateSimpleRandomUint64();
		if (this->protocol == "tcp") {
			this->protocol_id = 1;
			this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
			this->server_addr = get_tcp_server_address(address.c_str(), port);
			return;
		}
		if (this->protocol == "udp") {
			this->protocol_id = 2;
			this->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
			this->server_addr = udp_get_server_address(address.c_str(), port);
			return;
		}
		if (this->protocol == "udpr") {
			this->protocol_id = 3;
			this->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
			this->server_addr = udp_get_server_address(address.c_str(), port);
			return;
		}
	}

	void get_data() {
		// Determine the size of the data in std::cin
		if constexpr (DEBUG) {
			std::cout << "Getting data\n";
		}
		std::cin.seekg(0, std::ios::end);
		this->payload_length = std::cin.tellg();

		if constexpr (DEBUG) {
			std::cout << "Payload length: " << this->payload_length << "\n";
		}

		std::cin.seekg(0, std::ios::beg);

		// Allocate a new char array
		char *raw_data = new char[payload_length];

		// Read the data from std::cin into the char array
		std::cin.read(raw_data, payload_length);

		// Convert the raw char array to a shared_ptr<char>
		this->data = std::shared_ptr<char>(raw_data, std::default_delete<char[]>());
	}

	void establish_connection() {
		if (this->protocol == "tcp") {
			if (connect(this->socket_fd, (struct sockaddr *) &this->server_addr, sizeof(this->server_addr)) < 0) {
				throw std::runtime_error("ERROR: CLIENT: connect failed\n");
			}
			if constexpr (DEBUG) {
				std::cout << "Client connected\n";
			}
		} else {
			// UDP connection - no connection needed
		}

		setSocketTimeout(this->socket_fd, MAX_WAIT);


		if constexpr (DEBUG) {
			std::cout << "Client sending conn packet\n";
		}
		CONN conn_packet{.type = 1, .session_id = this->session_id, .protocol_id = protocol_id, .payload_length = ntohl(this->payload_length)};
		send(&conn_packet, sizeof(conn_packet));
		if constexpr (DEBUG) {
			std::cout << "Client sent conn packet with session_id: " << conn_packet.session_id << "\n";
		}
		CONACC conacc_packet;
		receive(&conacc_packet, sizeof(conacc_packet));

		if (conacc_packet.type != 2) {
			throw std::runtime_error("ERROR: CLIENT: Received CONRJT packet instead of CONACC\n");
		}
		if (conacc_packet.session_id != this->session_id) {
			throw std::runtime_error("ERROR: CLIENT: Received CONACC packet with incorrect session_id\n");
		}
		if constexpr (DEBUG) {
			std::cout << "Client received conacc packet with session_id: " << conacc_packet.session_id << "\n";
		}
	}

	void send_data() {
		while (payload_length > 0) {
			DATA_HEADER data_packet{.type = 4, .session_id = this->session_id, .packet_number = ntohs(this->cur_packet_number), .data_length = ntohs((uint32_t) std::min(MAX_DATA_SIZE, payload_length))};
			if (this->protocol == "tcp"){
				send(&data_packet, sizeof(data_packet));
				if constexpr (DEBUG) {
					std::cout << "Client sent data header with session_id: " << data_packet.session_id << " and packet number: " << data_packet.packet_number << "\n";
				}
				send(data.get() + (cur_packet_number * MAX_DATA_SIZE), ntohs(data_packet.data_length));
			}
			else {
				char tmp[MAX_DATA_SIZE + sizeof(data_packet)];
				memcpy(tmp, &data_packet, sizeof(data_packet));
				memcpy(tmp + sizeof(data_packet), data.get() + (cur_packet_number * MAX_DATA_SIZE), data_packet.data_length);
				send(tmp, sizeof(data_packet) + data_packet.data_length);
			}


			if constexpr (DEBUG) {
				std::cout << "Client sent data packet with session_id: " << data_packet.session_id << " and packet number: " << data_packet.packet_number << "\n";
			}
			if (this->protocol == "udpr"){
				ACC acc_packet;
				if constexpr (DEBUG) {
					std::cout << "Client receiving acc packet\n";
				}
				receive(&acc_packet, sizeof(acc_packet));
				if constexpr (DEBUG) {
					std::cout << "Client received acc packet with session_id: " << acc_packet.session_id << " and packet type=" << acc_packet.type << "\n";
				}
				if (acc_packet.type != 5) {
					throw std::runtime_error("ERROR: CLIENT: Received RJT packet instead of ACC\n");
				}
				if (acc_packet.session_id != this->session_id) {
					throw std::runtime_error("ERROR: CLIENT: Received ACC packet with incorrect session_id\n");
				}
				if (acc_packet.packet_number != ntohs(this->cur_packet_number)) {
					throw std::runtime_error("ERROR: CLIENT: Received ACC packet with incorrect packet_number\n");
				}
				if constexpr (DEBUG) {
					std::cout << "Client received acc packet with session_id: " << acc_packet.session_id << " and packet type=" << acc_packet.type << "\n";
				}
			}

			this->cur_packet_number++;
			payload_length -= std::min(MAX_DATA_SIZE, payload_length);
		}
	}

	void receive_confirmation() {
		RCVD rcvd_packet;
		receive(&rcvd_packet, sizeof(rcvd_packet));
		if (rcvd_packet.type != 7) {
			throw std::runtime_error("ERROR: CLIENT: Received RJT packet instead of RCVD\n");
		}
		if constexpr (DEBUG) {
			std::cout << "Client received rcvd packet with session_id: " << rcvd_packet.session_id << " and packet type=" << rcvd_packet.type << "\n";
		}
	}

	void close_connection() {
		return;
	}

	void start() {
		this->get_data();
		this->establish_connection();
		this->send_data();
		this->receive_confirmation();
		this->close_connection();
	}
};