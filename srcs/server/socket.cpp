
// #define _GNU_SOURCE   // 一部のオペレーティング・システムでは、_GNU_SOURCEの定義は不要。(https://docs.oracle.com/cd/E39368_01/porting/ch03s06.html)
#include "socket.hpp"

namespace ft
{
	Socket::Socket() : sockfd_vec_(), closedfd_vec_(), poll_fd_vec_(), last_recieve_time_map_(),
		msg_to_send_map_(), fd_to_port_map_(), used_fd_set_(), keep_connect_time_len_(30)
	{
	}

	Socket::~Socket()
	{
		closeAllSocket_();
	}

	Socket::Socket(const Socket& src) : sockfd_vec_(src.sockfd_vec_), closedfd_vec_(src.closedfd_vec_),
		poll_fd_vec_(src.poll_fd_vec_), last_recieve_time_map_(src.last_recieve_time_map_),
		msg_to_send_map_(src.msg_to_send_map_), fd_to_port_map_(src.fd_to_port_map_),
		used_fd_set_(src.used_fd_set_), keep_connect_time_len_(src.keep_connect_time_len_)
	{
	}

	Socket& Socket::operator=(const Socket& rhs) {
		sockfd_vec_ = rhs.sockfd_vec_;
		closedfd_vec_ = rhs.closedfd_vec_;
		poll_fd_vec_ = rhs.poll_fd_vec_;
		last_recieve_time_map_ = rhs.last_recieve_time_map_;
		msg_to_send_map_ = rhs.msg_to_send_map_;
		fd_to_port_map_ = rhs.fd_to_port_map_;
		used_fd_set_ = rhs.used_fd_set_;
		keep_connect_time_len_ = rhs.keep_connect_time_len_;
		return (*this);
	}

	Socket::RecievedMsg::RecievedMsg()
		: content(""), client_id(0), port(0)
	{
	}

	Socket::RecievedMsg::RecievedMsg(const RecievedMsg& src)
		: content(src.content), client_id(src.client_id), port(src.port)
	{
	}

	Socket::RecievedMsg::RecievedMsg(const std::string content, const int client_id, in_port_t port)
		: content(content), client_id(client_id), port(port)
	{
	}

	Socket::RecievedMsg Socket::RecievedMsg::operator=(const Socket::RecievedMsg &other)
	{
		if (this == &other)
			return (*this);

		content = other.content;
		client_id = other.client_id;
		port = other.port;
		return (*this);
	}
	
	Socket::RecievedMsg::~RecievedMsg()
	{
	}

	void Socket::setup(const std::vector<ServerConfig> &server_config_vec)
	{
		struct sockaddr_in server_sockaddr;
		struct pollfd poll_fd;
		std::set<unsigned int> boundPorts;
		unsigned int listenPort = 0;

		for (size_t i = 0; i < server_config_vec.size(); ++i)
		{
			listenPort = server_config_vec[i].getListen();

			// if port is already bound to socket, skip
			if (boundPorts.find(listenPort) != boundPorts.end())
				continue ;

			sockfd_vec_.push_back(socket(AF_INET, SOCK_STREAM, 0));
			if (sockfd_vec_.back() < 0)
				throw SetUpFailException("Error: socket()");

			set_sockaddr_(server_sockaddr, "127.0.0.1", listenPort);
			std::cout << "127.0.0.1 " << listenPort << " " << sockfd_vec_.back() << std::endl;

			const int opt = 1;
			if (setsockopt(sockfd_vec_.back(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
				throw SetUpFailException("Error: setsockopt()");

			if (setsockopt(sockfd_vec_.back(), SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
				throw SetUpFailException("Error: setsockopt()");

			try {
				set_nonblock_(sockfd_vec_.back());
			} catch (const std::exception& e) {
				throw SetUpFailException(e.what());
			}

			fd_to_port_map_[sockfd_vec_.back()] = listenPort;

			if (bind(sockfd_vec_.back(), (struct sockaddr *)&server_sockaddr,
					sizeof(server_sockaddr)) < 0)
				throw SetUpFailException("Error: bind()");

			boundPorts.insert(listenPort);

			if (listen(sockfd_vec_.back(), SOMAXCONN) < 0)
				throw SetUpFailException("Error: listen()");

			poll_fd.fd = sockfd_vec_.back();
			poll_fd.events = POLLIN;
			poll_fd.revents = 0;
			poll_fd_vec_.push_back(poll_fd);
			last_recieve_time_map_[sockfd_vec_.back()] = -1;
		}
	}

	Socket::RecievedMsg Socket::recieve_msg()
	{
		
		print_event_debug();

		int poll_rslt = poll(&poll_fd_vec_[0], poll_fd_vec_.size(), 1000);	
		if (poll_rslt == -1) {
			// if the particular error is fd > max fd || no available memory
			// removed the last fd from poll list
			if (errno == EINVAL || errno == ENOMEM) {
				int last_fd = poll_fd_vec_.back().fd;
				// only close if not a port bound fd
				if (!ft::vecIncludes(sockfd_vec_, last_fd)) {
					close_fd_(last_fd, poll_fd_vec_.size() - 1);
					throw closedConnection(last_fd);
				}
			}
			// otherwise, just continue
			throw NoRecieveMsg();
		}

		for (size_t i = 0; poll_rslt > 0 && i < poll_fd_vec_.size(); ++i)
		{
			struct pollfd& client = poll_fd_vec_[i];
			if ((client.revents & POLLERR) == POLLERR)
			{
				std::cerr << "POLLERR: " << client.fd << std::endl;	
				close_fd_(client.fd, i);
				throw closedConnection(client.fd);
			}
			else if ((client.revents & POLLHUP) == POLLHUP)
			{
				std::cerr << "POLLHUP: " << client.fd << std::endl;
				close_fd_(client.fd, i);
				throw closedConnection(client.fd);
			}
			else if ((client.revents & POLLRDHUP) == POLLRDHUP)
			{
				std::cerr << "POLLRDHUP: " << client.fd << std::endl;
				close_fd_(client.fd, i);	
				throw closedConnection(client.fd);
			}
			else if ((client.revents & POLLIN) == POLLIN)
			{
				client.revents = 0;
				if (used_fd_set_.count(client.fd))
				{
					client.revents = 0;
					return (recieve_msg_from_connected_client_(client.fd, i));
				}
				else
				{
					int connection = accept(client.fd, NULL, NULL);
					if (connection == -1)
						throw NoRecieveMsg();

					try {
						set_nonblock_(connection);
					} catch (const std::exception& e) {
						throw serverInternalError(connection);
					}
					register_new_client_(connection);
					fd_to_port_map_[connection] = fd_to_port_map_[client.fd];
					throw recieveMsgFromNewClient(*(--used_fd_set_.end()));
				}
			}
			else if ((client.revents & POLLOUT) == POLLOUT)
			{
				client.revents = 0;
				unsigned int	response_code = msg_to_send_map_[client.fd].first;
				std::string&	msg_to_send = msg_to_send_map_[client.fd].second;

				ssize_t sent_num = send(client.fd, msg_to_send.c_str(),
									   msg_to_send.size(), 0);
				if (sent_num != -1 && static_cast<size_t>(sent_num) != msg_to_send.size())
					msg_to_send.erase(0, sent_num);
				else {
					msg_to_send_map_.erase(client.fd);
					if (sent_num == -1 || response_code >= 400) {
							close_fd_(client.fd, i);
							throw closedConnection(client.fd);
					}
					client.events = POLLIN;
				}
				last_recieve_time_map_[client.fd] = time(NULL);
			}
		}
		// throw recieveMsgException();	// pollにタイムアウトを設定するので除外
		throw NoRecieveMsg();	
	}

	void Socket::send_msg(int fd, unsigned int response_code, const std::string msg)
	{
		std::pair<unsigned int, std::string>& message = msg_to_send_map_[fd];
		if (message.first != response_code)
			message.first = response_code;
		message.second.append(msg);

		size_t index = 0;
		for (; index < poll_fd_vec_.size() && poll_fd_vec_[index].fd != fd; ++index) { ; }

		poll_fd_vec_[index].events = POLLOUT;
	}

	std::vector<int>& Socket::check_keep_time_and_close_fd()
	{
		time_t current_time = time(NULL);
		time_t tmp_last_recieve_time;

		std::cout << current_time << std::endl;

		for (size_t i = 0; i < poll_fd_vec_.size(); ++i)
		{
			tmp_last_recieve_time = last_recieve_time_map_[poll_fd_vec_[i].fd];
			if (tmp_last_recieve_time != (time_t)-1)
			{ // fd made by accept(), not sockfd
				if (current_time - tmp_last_recieve_time > keep_connect_time_len_)
				{
					std::cerr << "keep alive close" << std::endl;
					closedfd_vec_.push_back(poll_fd_vec_[i].fd);
					close_fd_(poll_fd_vec_[i].fd, i);
				}
			}
		}
		return(closedfd_vec_);
	}

	void Socket::register_new_client_(int connection)
	{
		struct pollfd poll_fd;
		poll_fd.fd = connection;
		poll_fd.events = POLLIN;
		poll_fd.revents = 0;
		poll_fd_vec_.push_back(poll_fd);
		used_fd_set_.insert(connection);

		last_recieve_time_map_[connection] = time(NULL);
	}

	Socket::RecievedMsg Socket::recieve_msg_from_connected_client_(int connection, size_t i_poll_fd)
	{
		char buf[BUFFER_SIZE + 1];

		last_recieve_time_map_[connection] = time(NULL);

		ssize_t recv_ret = recv(connection, buf, BUFFER_SIZE, 0);
		if (recv_ret == -1)
			throw serverInternalError(connection);
		if (recv_ret == 0) {
			close_fd_(connection, i_poll_fd);
			throw closedConnection(connection);
		}

		buf[recv_ret] = '\0';
		return (RecievedMsg(std::string(buf), connection, fd_to_port_map_[connection]));
	}

	void Socket::remove_cgi_fd(int fd) {
		size_t index = 0;
		for (; index < poll_fd_vec_.size() && poll_fd_vec_[index].fd != fd; ++index) { ; }
		std::cout << "REMOVING FD: " << poll_fd_vec_[index].fd << std::endl;
		poll_fd_vec_.erase(poll_fd_vec_.begin() + index);
		used_fd_set_.erase(fd);
	}


	void Socket::close_fd_(const int fd, const int i_poll_fd)
	{
		close(fd);
		poll_fd_vec_.erase(poll_fd_vec_.begin() + i_poll_fd);
		used_fd_set_.erase(fd);
		fd_to_port_map_.erase(fd);
	}

	void Socket::closeAllSocket_()
	{
		for (size_t i = 0; i < poll_fd_vec_.size(); ++i) {
			close(poll_fd_vec_[i].fd);
		}
		poll_fd_vec_.clear();
		used_fd_set_.clear();
		fd_to_port_map_.clear();
	}

	void Socket::set_sockaddr_(struct sockaddr_in &server_sockaddr, const char *ip, const in_port_t port)
	{
		memset(&server_sockaddr, 0, sizeof(struct sockaddr_in));
		server_sockaddr.sin_family = AF_INET;
		server_sockaddr.sin_port = htons(port);
		server_sockaddr.sin_addr.s_addr = inet_addr(ip);
	}

	void Socket::set_nonblock_(int fd)
	{
		int flags = fcntl(fd, F_GETFL, 0);
		if (flags == -1)
			throw std::runtime_error("Error: setsockopt()");
		if ((flags & O_NONBLOCK) != O_NONBLOCK) {	
			if (fcntl(fd, F_SETFL, (flags | O_NONBLOCK)) == -1)
				throw std::runtime_error("Error: setsockopt()");
		}
	}

	Socket::SetUpFailException::SetUpFailException(const std::string err_msg)
		: err_msg(err_msg)
	{
	}

	Socket::SetUpFailException::~SetUpFailException() throw()
	{
	}

	const char *Socket::SetUpFailException::what() const throw()
	{
		return (err_msg.c_str());
	}

	const char *Socket::RecieveMsgException::what() const throw()
	{
		return ("Error: recieve msg fail, shouldn't happen");
	}

	Socket::recieveMsgFromNewClient::recieveMsgFromNewClient(const int client_id)
		: client_id(client_id)
	{
	}

	Socket::closedConnection::closedConnection(const int client_id)
		: client_id(client_id)
	{
	}

	Socket::serverInternalError::serverInternalError(const int client_id)
		: client_id(client_id)
	{
	}

	Socket::readCGIfd::readCGIfd(const int cgi_fd)
		: cgi_fd(cgi_fd)
	{
	}

	void Socket::print_event_debug() {
		std::cout << "poll_fd_vec_.size(): " << poll_fd_vec_.size() << std::endl;
		for (size_t i = 0; i < poll_fd_vec_.size(); ++i) {
			std::cout << poll_fd_vec_[i].fd << " e" << poll_fd_vec_[i].events << " re" << poll_fd_vec_[i].revents;
			std::cout << (i < poll_fd_vec_.size() - 1 ? " : " : "");
		}
		std::cout << std::endl;
	}
}
