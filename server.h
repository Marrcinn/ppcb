#pragma once
#include <arpa/inet.h>
#include <endian.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <numeric>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"

class Server {
private:
	sockaddr_in server_addr;
	sockaddr_in client_addr;
	int socket_fd;
	int client_fd;
	std::string protocol;
	std::shared_ptr<char> buff;
	uint64_t current_packet_number = 0;
	uint64_t session_id;
	uint64_t payload_length;

	void send(void *data, uint32_t size) {
		if (this->protocol == "tcp") {
			tcpSend(this->client_fd, data, size);
		} else {
			udpSend(this->socket_fd, data, size, &this->client_addr);
		}
	}

	void receive(void *data, uint32_t size) {
		if (this->protocol == "tcp") {
			tcpReceive(this->client_fd, data, size);
		} else {
			udpReceive(this->socket_fd, data, size, &this->client_addr);
		}
	}

protected:
	// Sets up the connection with
	void get_connection() {
		if (this->protocol != "tcp") {
			this->protocol = "udp";
			return;
		}
		if (listen(socket_fd, 5) < 0) {
			throw std::runtime_error("ERROR: listen failed\n");
		}
		if constexpr (DEBUG) {
			std::cout << "Server listening\n";
		}
	}

	void send_connacc() {
		CONACC conacc_packet{.type = 2, .session_id = this->session_id};
		send(&conacc_packet, sizeof(conacc_packet));
		if constexpr (DEBUG) {
			std::cout << "Server sent conacc packet with session_id: " << conacc_packet.session_id << "\n";
		}
	}

	void receive_and_send_confirmations() {
		if constexpr (DEBUG) {
			std::cout << "start of function: receive_and_send_confirmations()\n";
		}
		if (this->protocol == "tcp") {
			socklen_t client_addr_len = sizeof(this->client_addr);
			this->client_fd = accept(socket_fd, (struct sockaddr *) &this->client_addr, &client_addr_len);
			if (this->client_fd < 0) {
				throw std::runtime_error("ERROR: accept failed\n");
			}
			if constexpr (DEBUG) {
				std::cout << "Server accepted\n";
			}
		}

		CONN conn_packet;
		if constexpr (DEBUG) {
			std::cout << "Server receiving connection packet\n";
		}
		receive(&conn_packet, sizeof(conn_packet));
		if (conn_packet.type != 1) {
			throw std::runtime_error("ERROR: invalid packet type\n");
		}
		if constexpr (DEBUG) {
			std::cout << "Server received connection packet with type: " << conn_packet.type << "\n";
		}
		if (this->protocol == "tcp") {
			if (conn_packet.protocol_id != 1) {
				throw std::runtime_error("ERROR: invalid protocol id\n");
			}
		} else {
			if (conn_packet.protocol_id != 2 && conn_packet.protocol_id != 3) {
				throw std::runtime_error("ERROR: invalid protocol id\n");
			}
		}
		this->session_id = conn_packet.session_id;
		this->payload_length = conn_packet.payload_length;
		this->current_packet_number = 0;
		if constexpr (DEBUG) {
			std::cout << "Server received connection packet with session_id: " << conn_packet.session_id << " and payload length " << this->payload_length << "\n";
		}
		this->send_connacc();
	}

	void send_rjt_packet() {
		RJT rjt_packet{.type = 6, .session_id = this->session_id, .packet_number = this->current_packet_number};
		send(&rjt_packet, sizeof(rjt_packet));
	}

	// Validates data header. Throws runtime_error if the header is invalid for current connection.
	void get_and_validate_data_header(DATA_HEADER &data_header) {
		receive(&data_header, sizeof(data_header));
		if constexpr (DEBUG) {
			std::cout << "Server got DATA_HEADER packet with type: " << data_header.type << " the expected packet size is " << data_header.data_length << " and session_id: " << data_header.session_id << "\n";
		}
		if (data_header.type != 4) {
			send_rjt_packet();
			throw std::runtime_error("ERROR: invalid packet type\n");
		}
		if (data_header.session_id != this->session_id) {
			// If we get a packet with a different session_id (different client), we just send the rjt packet and do not throw an error
			// as not to disrupt the connection with the current client
			send_rjt_packet();
		}
		if (data_header.packet_number != this->current_packet_number++) {
			current_packet_number--;
			send_rjt_packet();
			throw std::runtime_error("ERROR: invalid packet number\n");
		}
	}

	void send_acc(DATA_HEADER &data_header) {
		if (this->protocol == "udpr") {
			ACC acc_packet{.type = 5, .session_id = data_header.session_id, .packet_number = data_header.packet_number};
			send(&acc_packet, sizeof(acc_packet));
		}
	}


	void receive_data() {
		while (payload_length > 0) {
			DATA_HEADER data_header;
			this->get_and_validate_data_header(data_header);
			if (data_header.data_length > payload_length) {
				throw std::runtime_error("ERROR: sum of data_lengths exceed payload_length (current data length is " + std::to_string(data_header.data_length) + " and remaining payload_length is " + std::to_string(payload_length) + ")\n");
			}
			payload_length -= data_header.data_length;

			uint32_t current_data_length = 0;
			while (current_data_length < data_header.data_length) {
				if constexpr (DEBUG) {
					std::cout << "Server receiving data of max size " << data_header.data_length - current_data_length << "\n";
				}
				receive(buff.get(), data_header.data_length - current_data_length);
				current_data_length += data_header.data_length - current_data_length;
			}
			std::cout << std::string(buff.get(), data_header.data_length);
			send_acc(data_header);
		}
	}

	void close_connection() {
		close(this->client_fd);
		this->payload_length = 0;
		this->current_packet_number = 0;
		this->session_id = 0;
	}

	void confirm_data() {
		RCVD rcvd_packet{.type = 7, .session_id = this->session_id};
		send(&rcvd_packet, sizeof(rcvd_packet));
		if constexpr (DEBUG) {
			std::cout << "Server sent rcvd packet with session_id: " << rcvd_packet.session_id << "\n";
		}
	}

public:
	Server(std::string protocol, int port) : buff{new char[64000]} {
		if constexpr (DEBUG) {
			std::cout << "Server starting\n";
		}
		this->protocol = protocol;
		if (this->protocol != "tcp" && this->protocol != "udp") {
			throw std::runtime_error("ERROR: invalid protocol. Protocols supported: <tcp>, <udp>\n");
		}
		if (this->protocol == "tcp") {
			this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
			this->server_addr.sin_family = AF_INET;
			this->server_addr.sin_port = htons(port);
			this->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			if (bind(socket_fd, (struct sockaddr *) &this->server_addr, sizeof(this->server_addr)) < 0) {
				throw std::runtime_error("ERROR: bind failed\n");
			}
			if constexpr (DEBUG) {
				std::cout << "Server bound\n";
			}
		} else {
			this->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
			this->server_addr.sin_family = AF_INET;
			this->server_addr.sin_port = htons(port);
			this->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			if (bind(socket_fd, (struct sockaddr *) &this->server_addr, sizeof(this->server_addr)) < 0) {
				throw std::runtime_error("ERROR: bind failed\n");
			}
			if constexpr (DEBUG) {
				std::cout << "Server bound\n";
			}
		}
	}
	void run() {
		while (true) {
			this->get_connection();
			// This is the part where the server communicates with clients
			try {
				this->receive_and_send_confirmations();
				this->receive_data();
				this->confirm_data();
			} catch (std::exception &e) {
				std::cerr << e.what();
				close(this->client_fd);
			}
			this->close_connection();
		}
	}
};