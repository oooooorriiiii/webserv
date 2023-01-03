#ifndef FT_SOCKET_HPP
#define FT_SOCKET_HPP

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <poll.h>
#include <time.h>

#include <errno.h>
#include <stdio.h>

#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <fcntl.h>
#include <sstream>

#include "../config/Config.hpp"
#include "../utils/utils.hpp"

#define BUFFER_SIZE 10

namespace ft
{

	class Socket
	{
	public:

		typedef std::map<int, std::pair<unsigned int, std::string> >	message_map;
		// canonical
		Socket();
		virtual ~Socket();

		class RecievedMsg
		{
		public:
			RecievedMsg();
			RecievedMsg(const RecievedMsg& src);
			RecievedMsg operator=(const RecievedMsg &other);
			RecievedMsg(const std::string content, const int client_id, in_port_t port);
			~RecievedMsg();
			std::string content;
			int client_id;
			in_port_t port;
		};

		void setup(const std::vector<ServerConfig> &server_config);
		RecievedMsg recieve_msg();
		void send_msg(int fd, unsigned int response_code, const std::string msg);
		std::vector<int>& check_keep_time_and_close_fd();
		void close_fd_(const int fd, const int i_poll_fd);
		void register_new_client_(int fd, bool is_cgi);

		class SetUpFailException : public std::exception
		{
		public:
			SetUpFailException(const std::string err_msg);
			virtual ~SetUpFailException() throw();
			const char *what() const throw();
			const std::string err_msg;
		};

		class RecieveMsgException : public std::exception
		{
		public:
			const char *what() const throw();
		};

		class recieveMsgFromNewClient : public std::exception
		{
		public:
			recieveMsgFromNewClient(const int client_id);
			const int client_id;
		};

		class closedConnection : public std::exception
		{
		public:
			closedConnection(const int client_id);
			const int client_id;
		};

		class serverInternalError : public std::exception
		{
		public:
			serverInternalError(const int fd);
			const int fd_;
		};

		class readCGIfd : public std::exception
		{
		public:
			readCGIfd(const int cgi_fd);
			const int cgi_fd_;
		};

		class NoRecieveMsg : public std::exception
		{
		};

	private:
		Socket(const Socket& src);
		Socket& operator=(const Socket& rhs);

		std::vector<int> sockfd_vec_;
		std::vector<int> closedfd_vec_;
		std::vector<struct pollfd> poll_fd_vec_;
		std::map<int, time_t> last_recieve_time_map_; // sockfd => -1
		message_map		msg_to_send_map_;
		std::map<int, in_port_t> fd_to_port_map_;

		std::set<int> used_fd_set_;

		time_t keep_connect_time_len_;
		int cgi_;

		RecievedMsg recieve_msg_from_connected_client_(int client_fd, size_t i_poll_fd);
		RecievedMsg recieve_msg_from_cgi_(int cgi_fd, size_t i_poll_fd);
	
		void closeAllSocket_();
		void set_sockaddr_(struct sockaddr_in &server_sockaddr, const char *ip, const in_port_t port);
		void set_nonblock_(int fd);
		void print_event_debug();
	};

}

#endif
