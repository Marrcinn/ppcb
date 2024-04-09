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
	uint64_t payload_length;


public:
	Client(std::string protocol, std::string address, int port) {
		this->protocol = protocol;
		this->session_id = generateSimpleRandomUint64();
		if (this->protocol == "tcp") {
			this->protocol_id = 1;
			this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
			this->server_addr = get_tcp_server_address(address.c_str(), port);
		}
	}

	void get_data() {
		// Determine the size of the data in std::cin
		if constexpr (DEBUG){
			std::cout << "Getting data\n";
		}
		std::cin.seekg(0, std::ios::end);
		this->payload_length = std::cin.tellg();

		if constexpr (DEBUG){
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
				throw std::runtime_error("ERROR: connect failed\n");
			}
			if constexpr (DEBUG) {
				std::cout << "Client connected\n";
			}
		}


		CONN conn_packet{.type = 1, .session_id = this->session_id, .protocol_id = protocol_id, .payload_length = this->payload_length};
		if (send(this->socket_fd, &conn_packet, sizeof(conn_packet), 0) < 0) {
			throw std::runtime_error("ERROR: send failed\n");
		}
		if constexpr (DEBUG) {
			std::cout << "Client sent conn packet with session_id: " << conn_packet.session_id << "\n";
		}
		CONACC conacc_packet;
		if (recv(this->socket_fd, &conacc_packet, sizeof(conacc_packet), 0) < 0) {
			throw std::runtime_error("ERROR: recv failed\n");
		}
		if constexpr (DEBUG) {
			std::cout << "Client received conacc packet with session_id: " << conacc_packet.session_id << "\n";
		}
	}

	void send_data() {
		while (payload_length > 0) {
			DATA_HEADER data_packet{.type = 4, .session_id = this->session_id, .packet_number = this->cur_packet_number, .data_length = (uint32_t) std::min(MAX_DATA_SIZE, payload_length)};
			if (send(this->socket_fd, &data_packet, sizeof(data_packet), 0) < 0) {
				throw std::runtime_error("ERROR: send failed\n");
			}
			if constexpr (DEBUG) {
				std::cout << "Client sent data_header packet with session_id: " << data_packet.session_id << " and data_length: " << data_packet.data_length << "\n";
			}
			if (send(this->socket_fd, this->data.get() + this->cur_packet_number * MAX_DATA_SIZE, data_packet.data_length, 0) < 0) {
				throw std::runtime_error("ERROR: send failed\n");
			}
			if constexpr (DEBUG) {
				std::cout << "Client sent data packet with session_id: " << data_packet.session_id << " and data_length: " << data_packet.data_length << "\n";
			}
			this->cur_packet_number++;
			payload_length -= std::min(MAX_DATA_SIZE, payload_length);
		}
	}

	void receive_confirmation(){
		RCVD rcvd_packet;
		if (recv(this->socket_fd, &rcvd_packet, sizeof(rcvd_packet), 0) < 0) {
			throw std::runtime_error("ERROR: recv failed\n");
		}
		if (rcvd_packet.type != 7) {
			throw std::runtime_error("ERROR: Received RJT packet instead of RCVD\n");
		}
		if constexpr (DEBUG) {
			std::cout << "Client received rcvd packet with session_id: " << rcvd_packet.session_id << " and packet type="<< rcvd_packet.type <<"\n";
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