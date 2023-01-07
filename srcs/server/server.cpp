#include "server.hpp"
#include "../httpMethod/HTTPProcess.hpp"

namespace ft
{
	Server::Server(const std::string config_path) : server_config_list_(), socket_(),
		serverChild_map_(), default_serverChild_map_(), httpRequest_pair_map_(),
		cgi_client_socket_(std::make_pair(-1, -1))
	{
		import_config_(config_path);
		socket_.setup(server_config_list_);
		create_serverChild_map_();
		dfltLocConf = CreateDfltLocConf();
	}
	Server::~Server()
	{
	}

	void Server::start_server()
	{
		while (1)
		{
			recieve_request_();	
		}
	}

	LocationConfig	Server::CreateDfltLocConf() {
		LocationConfig dfltLoc;
		std::set<std::string> allow_method;
		std::vector<std::string> index;

		allow_method.insert("GET");
		index.push_back("index.html");

		dfltLoc.setUri("/");
		dfltLoc.setAlias("./var/www/inception_server/html/");
		dfltLoc.setAutoindex(false);
		dfltLoc.setAllowMethod(allow_method);
		dfltLoc.addIndex(index);
		
		return(dfltLoc);
	}

	ServerConfig	Server::CreateDfltServConf(const std::string& host, unsigned int port) {
		ServerConfig	dfltSrv;
		ServerConfig::err_page_map err_page;

		err_page.insert(std::make_pair(404, "/error404.html"));

		dfltSrv.setServerName(host);
		dfltSrv.setListen(port);
		dfltSrv.setClientMaxBodySize(1000000);
		dfltSrv.addErrorPage(err_page);
		dfltSrv.addLocationConfig(dfltLocConf.getUri(), dfltLocConf);

		return (dfltSrv);
	}

	void Server::import_config_(const std::string config_path)
	{
		ConfigParser configParser;
		server_config_list_ = configParser.readFile(config_path).getServerConfig();
		//print_server_config();
	}

	void Server::create_serverChild_map_()
	{
		std::vector<ServerConfig>::const_iterator end = server_config_list_.end();
		for (std::vector<ServerConfig>::const_iterator it = server_config_list_.begin(); it != end; ++it)
		{
			std::pair<std::string, in_port_t> key_to_insert = std::make_pair((*it).getServerName(), (in_port_t)(*it).getListen());
			serverChild_map_.insert(std::make_pair(key_to_insert, ServerChild(*it)));
			if (default_serverChild_map_.count((in_port_t)(*it).getListen()) == 0)
				default_serverChild_map_.insert(std::make_pair((in_port_t)(*it).getListen(), (*(serverChild_map_.find(key_to_insert))).second));
		}
	}	

	void Server::recieve_request_()
	{
		Socket::RecievedMsg recieved_msg;
		unsigned int		response_code;
		std::string			response;

		try
		{
			remove_timeout_clients_();

			recieved_msg = socket_.recieve_msg();

			print_debug_(recieved_msg);

			// return from this loop if cgi message has been sent
			if (handle_cgi(recieved_msg))
				return ;

			ServerChild& serverChild = httpRequest_pair_map_[recieved_msg.client_id].second;
			process_msg_(serverChild, recieved_msg);

			if (serverChild.Get_parse_status() == complete) {
				std::cout << "PATH: " << serverChild.Get_path() << std::endl;
				std::cout << "BODY RECEIVED: ";
				std::cout << serverChild.Get_body() << std::endl;

				response_code = serverChild.Get_response_code();

				if (response_code >= 300 && response_code < 400) {
					std::cout << "request is redirct\n";
					response = CreateRedirectResponse(response_code, serverChild.Get_path());
				} else if (response_code >= 400) {
					std::cout << "request is bad\n";
					ServerConfig::err_page_map error_pages = serverChild.Get_server_config().getErrorPage();
					std::set<std::string> allow_method = serverChild.Get_location_config().getAllowMethod();
					response = CreateErrorResponse(response_code, error_pages, allow_method);
				} else {
					std::cout << "request is good\n";
                	response = http_process(serverChild);
					response_code = serverChild.Get_response_code();
				}
                std::cout << "RESPONSE: \n" << response << std::endl; // Debug

                socket_.send_msg(recieved_msg.client_id, response_code, response);
				httpRequest_pair_map_.erase(recieved_msg.client_id);
			}
		}
		catch (const ft::Socket::recieveMsgFromNewClient &new_client)
		{
			httpRequest_pair_map_[new_client.client_id];
		}
		catch (const ft::Socket::closedConnection &deleted_client)
		{
			httpRequest_pair_map_.erase(deleted_client.client_id);
		}
		catch (const ft::Socket::readCGIfd &readCGIexc)
		{
			socket_.register_new_client_(readCGIexc.cgi_fd_, true);
			// client socket fd
			cgi_client_socket_.first = recieved_msg.client_id;
			// cgi socket fd
			cgi_client_socket_.second = readCGIexc.cgi_fd_;
		}
		catch (const ft::Socket::NoRecieveMsg &e)
		{
			std::cerr << "no msg recieved" << std::endl;
		}
		catch (const ft::Socket::serverInternalError &srvrExc)
		{	
			int client_fd = srvrExc.fd_;

			// The client_fd will be the fd received in the exception
			// except when that fd is the cgi_fd
			// the cgi_fd is already closed now, so we need to send the message to 
			// the client who originally requested the cgi operation
			// (cgi_client_socker.first)
			if (client_fd == cgi_client_socket_.second) { // cgi_fd
				client_fd = cgi_client_socket_.first; // client_fd
				cgi_client_socket_.first = -1;
				cgi_client_socket_.second = -1;
			}

			ServerChild& serverChild = httpRequest_pair_map_[client_fd].second;
			ServerConfig::err_page_map error_pages = serverChild.Get_server_config().getErrorPage();
			std::set<std::string> allow_method = serverChild.Get_location_config().getAllowMethod();

            socket_.send_msg(client_fd, 500, CreateErrorResponse(500, error_pages, allow_method));
		}
	}

	void Server::remove_timeout_clients_() {
		std::vector<int>& closedfd_vec = socket_.check_keep_time_and_close_fd();
		for (std::vector<int>::iterator it = closedfd_vec.begin(); it != closedfd_vec.end(); ++it) {
			httpRequest_pair_map_.erase(*it);
		}
		closedfd_vec.clear();
	}

	bool Server::handle_cgi(const Socket::RecievedMsg& recieved_msg) {
		int client_fd = cgi_client_socket_.first; 
		int cgi_fd = cgi_client_socket_.second; 
	
		// return false if this is not cgi_fd
		if (recieved_msg.client_id != cgi_fd) 
			return (false);

		// get connection information of the original client_fd
		ServerChild& serverChild = httpRequest_pair_map_[client_fd].second;
		std::string connection = get_connection(serverChild.Get_HTTPHead().GetHeaderFields());

		// send message that was read with poll->revent == HANGUP to original client
		send_cgi_msg_(client_fd, recieved_msg.content, connection);

		// prepare for new request to come in from the client
		httpRequest_pair_map_.erase(client_fd);
		cgi_client_socket_.first = -1;
		cgi_client_socket_.second = -1;
		return (true);
	}
				
	void	Server::process_msg_(ServerChild& serverChild, const Socket::RecievedMsg& recieved_msg) {

		HTTPHead& head = httpRequest_pair_map_[recieved_msg.client_id].first;

		try {
			if (head.GetParseStatus() != complete) {
				if (head.Parse(recieved_msg.content) == 0) {
					std::cout << "HEADER RECIEVED\n";
					head.ParseRequestURI();
					head.PrintRequest();
					serverChild = decide_serverChild_config_(head.GetHost(), recieved_msg.port);
					serverChild.SetUp(head);
					if (serverChild.Get_parse_status() != complete)
						serverChild.Parse("");
				}
			} else if (serverChild.Get_parse_status() != complete) {
				serverChild.Parse(recieved_msg.content);
			}
		} catch (const std::exception& e) {
			if (head.GetParseStatus() != complete) {
				serverChild.Set_response_code(head.GetResponseCode());
			}
			serverChild.Set_parse_status(complete);
			std::cout << "error while parsing http request: " << e.what() << std::endl;
		}	
	}

	ServerChild		Server::decide_serverChild_config_(const std::string& host, in_port_t port) {
        ServerChildMap::iterator confIt = serverChild_map_.find(std::make_pair(host, port));
		ServerChild server_child;
		ServerConfig::loc_conf_map	location_config_list;
		
        if (confIt != serverChild_map_.end()) {
			std::cout << "found serverChild by httpRequest host\n";
            server_child = confIt->second;
        } else {
			std::cout << "could not find serverchild, using default\n";
			DefaultServerChildMap::iterator it = default_serverChild_map_.find(port);
			if (it == default_serverChild_map_.end()) {
				std::cout << "No Default server Conf, creating webserv default server Config\n";
				server_child = ServerChild(CreateDfltServConf(host, port));
			} else {
            	server_child = it->second;
			}

        }

		ServerConfig& serverConf = server_child.Get_server_config();
		location_config_list = serverConf.getLocationConfigList();
		if (location_config_list.find("/") == location_config_list.end())
			serverConf.addLocationConfig(dfltLocConf.getUri(), dfltLocConf);
		return (server_child);
	}	

	void Server::send_cgi_msg_(int client_id, const std::string& content, const std::string& connection) {
  		/*
   		* Create response header
   		*/
		int response_status;
  		std::stringstream response_message_stream;
		std::string response_message_str;

  		response_message_stream << "HTTP/1.1 200 OK" << CRLF;
  		response_status = connection == "close" ? 400 : 200;
  		response_message_stream << "Server: " << "42webserv" << "/1.0" << CRLF;
  		response_message_stream << "Date: " << CreateDate() << CRLF;
  		response_message_stream << "Last-Modified: " << CreateDate() << CRLF;
  		response_message_stream << "Content-Type: " << "text/html" << CRLF;
  		response_message_stream << "Content-Length: " << content.size() << CRLF;
  		response_message_stream << "Connection: " << connection << CRLF;

	  	// send body
	  	response_message_stream << CRLF;

		response_message_stream << content;

		response_message_str = response_message_stream.str();
        socket_.send_msg(client_id, response_status, response_message_str);
	}

	void Server::print_server_config() {
		for (std::vector<ServerConfig>::iterator it = server_config_list_.begin(); it != server_config_list_.end(); ++it) {
			std::cout << "\t\t-----------SERVER-----------" << std::endl;
			it->print();
			for (ServerConfig::loc_conf_map::const_iterator lIt = it->getLocationConfigList().begin(); lIt != it->getLocationConfigList().end(); ++lIt) {
				std::cout << "\t------loc: " << lIt->first << " ------" << std::endl;
				lIt->second.print();
			}
		}
	}

	void Server::print_debug_(const Socket::RecievedMsg& recieved_msg) {
		std::cout << "httpRequest_pair_map_ size " << httpRequest_pair_map_.size() << std::endl;
		for (std::map<int, HTTPRequestPair>::iterator it = httpRequest_pair_map_.begin(); it !=	httpRequest_pair_map_.end(); ++it) {
			std::cout << it->first << " ";
		}
		std::cout << std::endl;
		std::cout << "===============================" << std::endl
				  << "port " << recieved_msg.port
				  << " - fd " << recieved_msg.client_id
				  << " - msg " << recieved_msg.content << std::endl
				  << "===============================" << std::endl;
	}
}
