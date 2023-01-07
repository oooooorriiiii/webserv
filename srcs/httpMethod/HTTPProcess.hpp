//
// Created by ymori on 22/12/17.
//

#ifndef WEBSERV_SRCS_HTTPMETHOD_HTTPPROCESS_HPP_
#define WEBSERV_SRCS_HTTPMETHOD_HTTPPROCESS_HPP_

#include "HTTPMethod.hpp"
#include "../HTTP/HTTPHead.hpp"

#include <iostream>

std::string http_process(ft::ServerChild& server_child);
std::string get_connection(const http_header_t& headers);
std::string createUploadFP(const std::string& filepath, const std::string path_parts);

#endif //WEBSERV_SRCS_HTTPMETHOD_HTTPPROCESS_HPP_
