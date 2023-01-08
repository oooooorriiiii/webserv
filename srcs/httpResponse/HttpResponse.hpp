//
// Created by yuumo on 2022/10/14.
//

#ifndef WEBSERV_SRCS_HTTPRESPONSE_HPP_
#define WEBSERV_SRCS_HTTPRESPONSE_HPP_

#define CRLF "\r\n"

#include <string>
#include "../config/ServerConfig.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

std::string GetResponseLine(int status_code);
std::string CreateDate();

std::string CreateSimpleResponseHeaders(int status_code);
std::string CreateSimpleResponseBody(const std::string& err);
std::string FileContentToStr(const std::string& path);

std::string CreateRedirectResponse(int status_code, const std::string& location);
std::string CreateErrorResponse(int status_code,
                            const ServerConfig::err_page_map& err_pages,
                            const std::set<std::string> &allow_methods);





#endif //WEBSERV_SRCS_HTTPRESPONSE_HPP_
