#include <iostream>

#include "common.h"
#include "server.h"

int main(int argc, char *argv[]) {
	if constexpr (DEBUG) {
		std::cout << "Server starting\n";
	}
	if (argc != 3) {
		std::cerr << "ERROR: usage" << argv[0] << " <protocol> <port>\n";
		return 1;
	}
	if (std::string(argv[1]) != "tcp" &&
	    std::string(argv[1]) != "udp" &&
	    std::string(argv[1]) != "udpr") {
		std::cerr
		    << "ERROR: invalid protocol. Protocols supported: <tcp>, <udp>, <udpr>\n";
		return 1;
	}
	try {
		if constexpr (DEBUG) {
			std::cout << "Server starting\n";
		}
		Server server{std::string(argv[1]), std::stoi(argv[2])};
		server.run();
	} catch (
	    const std::exception &e) {
		std::cerr << e.what();
		return 1;
	}

	return 0;
}