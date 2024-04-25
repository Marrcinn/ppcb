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

void validate_acc_packet(ACC &acc_packet) {
	acc_packet.packet_number = ntohll(acc_packet.packet_number);
	if constexpr (DEBUG) {
		std::cout << "Received acc packet: " <<
			  "type: " << (int) acc_packet.type <<
			  "session_id: " << acc_packet.session_id <<
			  "packet_number: " << acc_packet.packet_number <<
			  "\n";
	}
	if (acc_packet.type != 5) {
		throw std::runtime_error("ERROR: CLIENT: Received RJT packet instead of ACC\n");
	}
	if (acc_packet.session_id != this->session_id) {
		throw std::runtime_error("ERROR: CLIENT: Received ACC packet with incorrect session_id\n");
	}
	if (acc_packet.packet_number != this->cur_packet_number) {
		// No matter what the packet_number is, if it is invalid we retransmit the data.
		throw std::runtime_error("ERROR: CLIENT: Received ACC packet with incorrect packet_number\n");
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
		int buf_size = 64400; // By default we will read 64400 bytes and increase the buffer if needed.
		char *raw_data = new char[buf_size];
		int bytes_read = 0;
		while (true) {
			int n_read = read(STDIN_FILENO, raw_data + bytes_read, buf_size - bytes_read);
			if (n_read <= 0) {
				break;
			}
			bytes_read += n_read;
			if (bytes_read == buf_size) {
				buf_size *= 2;
				char *new_raw_data = new char[buf_size];
				memcpy(new_raw_data, raw_data, bytes_read);
				delete[] raw_data;
				raw_data = new_raw_data;
			}
		}
		payload_length = bytes_read;
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
		CONN conn_packet{.type = 1, .session_id = this->session_id, .protocol_id = protocol_id, .payload_length = htonll(this->payload_length)};
		CONACC conacc_packet;
		int retries = 0;
		while (retries <= MAX_RETRANSMITS){
			try {
				retries++;
				send(&conn_packet, sizeof(conn_packet));
				if constexpr (DEBUG) {
					std::cout << "Client sent conn packet with session_id: " << conn_packet.session_id << "\n";
				}
				receive(&conacc_packet, sizeof(conacc_packet));
				break;
			}
			catch (std::runtime_error &e){
				if (protocol == "udpr" && retries <= MAX_RETRANSMITS){
					continue;
				}
				throw e;
			}
		}

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
		int retries = 0;
		while (payload_length > 0) {
			try {
				DATA_HEADER data_packet{.type = 4, .session_id = this->session_id, .packet_number = htonll(this->cur_packet_number), .data_length = htonl((uint32_t) std::min(MAX_DATA_SIZE, payload_length))};
				if (this->protocol == "tcp") {
					send(&data_packet, sizeof(data_packet));
					send(data.get() + (cur_packet_number * MAX_DATA_SIZE), ntohl(data_packet.data_length));
				} else {
					char tmp[MAX_DATA_SIZE + sizeof(data_packet)];
					memcpy(tmp, &data_packet, sizeof(data_packet));
					memcpy(tmp + sizeof(data_packet), data.get() + (cur_packet_number * MAX_DATA_SIZE), ntohl(data_packet.data_length));
					send(tmp, sizeof(data_packet) + ntohl(data_packet.data_length));
					if (this->protocol == "udpr") {
						ACC acc_packet;
						if constexpr (DEBUG) {
							std::cout << "Client receiving acc packet\n";
						}
						receive(&acc_packet, sizeof(acc_packet));
						validate_acc_packet(acc_packet);
					}
				}
				if constexpr (DEBUG) {
					std::cout << "Client sent data packet with session_id: " << data_packet.session_id << " and packet number: " << ntohll(data_packet.packet_number) << "\n";
				}
			}
			catch (std::runtime_error &e) {
				if (this->protocol == "udpr") {
					retries++;
					if (retries > MAX_RETRANSMITS) {
						throw std::runtime_error("ERROR: CLIENT: Maximum number of retransmits reached\n");
					}
					continue;
				}
				throw e;
			}
			retries = 0; // As there was a success, we reset the number of retries
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
		close(this->socket_fd);
	}

	int start() {
		try {
			this->get_data();
			if constexpr (DEBUG) {
				std::cout << "Client got data\n";
			}
			this->establish_connection();
			if constexpr (DEBUG) {
				std::cout << "Client established connection\n";
			}
			this->send_data();
			if constexpr (DEBUG) {
				std::cout << "Client sent data\n";
			}
			this->receive_confirmation();
			if constexpr (DEBUG) {
				std::cout << "Client received confirmation\n";
			}
			this->close_connection();
			if constexpr (DEBUG) {
				std::cout << "Client closed connection\n";
			}
			return 0;
		}
		catch (std::runtime_error &e) {
			std::cerr << e.what();
			if constexpr (DEBUG) {
				std::cout << e.what() << "\n";
			}
			this->close_connection();

			return 1;
		}
	}
};