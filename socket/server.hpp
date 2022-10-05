#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <poll.h>

#include <errno.h>
#include <stdio.h>

#include <vector>

#include "socket.hpp"

namespace ft
{

	class Server
	{
	public:
		Server();							 // default conf
		Server(const std::string conf_path); // custom conf
		Server(std::vector<in_port_t> port_vec);

		void start_server();

	private:
		ft::Socket socket;
		void setup_();
		bool recieve_request_();
		std::map<int, std::string> http_request_map_;
	};

}
#endif
