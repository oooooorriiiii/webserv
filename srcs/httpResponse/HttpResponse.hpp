//
// Created by yuumo on 2022/10/14.
//

#ifndef WEBSERV_SRCS_HTTPRESPONSE_HPP_
#define WEBSERV_SRCS_HTTPRESPONSE_HPP_

#include <string>
#define CRLF "\r\n"

std::string GetResponseLine(int status_code);
std::string CreateDate();
std::string CreateErrorSentence(int status_code);
std::string CreateSimpleResponse(int status_code);
std::string CreateSimpleResponseBody(const std::string& err);

#endif //WEBSERV_SRCS_HTTPRESPONSE_HPP_
