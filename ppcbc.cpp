#include <iostream>
#include <stdexcept>
#include <string>
#include <cstdint>

#include "client.h"

int main(int argc, char *argv[]){
	if (argc != 4){
		std::cerr << "ERROR: usage" << argv[0] << " <protocol> <address> <port>\n";
	}
	std::string protocol = argv[1];
	std::string address = argv[2];
	int port;
	try {
		port = std::stoi(argv[3]);
	}
	catch (std::invalid_argument &e){
		std::cerr << "ERROR: invalid port. Ports must be integers\n";
		return 1;
	}
	catch (std::out_of_range &e){
		std::cerr << "ERROR: invalid port. Ports must be between 0 and 65535\n";
		return 1;
	}
	if (protocol != "tcp" && protocol != "udp" && protocol != "udpr"){
		std::cerr << "ERROR: invalid protocol. Protocols supported: <tcp>, <udp>, <udpr>\n";
		return 1;
	}
	if (port < 0 || port > 65535){
		std::cerr << "ERROR: invalid port. Ports must be between 0 and 65535\n";
		return 1;
	}
	try{
		Client client{protocol, address, port};
		client.start();
	}
	catch (const std::exception &e){
		std::cerr << e.what();
		return 1;
	}



	return 0;
}