//
// Created by ymori on 2022/10/18.
//

#ifndef WEBSERV_SRCS_HTTPMETHOD_HTTPMETHOD_H_
#define WEBSERV_SRCS_HTTPMETHOD_HTTPMETHOD_H_

#include <map>
#include <string>
#include "../HTTP/HTTPHead.hpp"
#include "../server/server.hpp"
#include "../server/socket.hpp"

typedef std::map<std::string, std::string> http_header_t;

int return1();

int do_put(std::string &response_message_str,
           const std::string &file_path,
           const std::string &http_body,
           const ServerConfig::err_page_map &err_pages,
           const std::string &connection,
           const std::string &upload_file_path,
           const std::set<std::string> &allow_method);


int do_get(std::string &response_message_str,
           const std::string &file_path,
           const ServerConfig::err_page_map &err_pages,
           const std::string &connection,
           bool autoindex,
           const std::string alias,
           const std::set<std::string> &allow_method);

int do_delete(std::string &response_message_str,
           const std::string &file_path,
           const ServerConfig::err_page_map &err_pages,
           const std::string &connection,
           const std::set<std::string> &allow_method);

int do_CGI(std::string &response_message_str,
           ft::ServerChild server_child,
           std::string file_path,
           std::string query_string,
           const ServerConfig::err_page_map &err_pages,
           const std::set<std::string> &allow_method);

#endif //WEBSERV_SRCS_HTTPMETHOD_HTTPMETHOD_H_
