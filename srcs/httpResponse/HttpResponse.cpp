//
// Created by yuumo on 2022/10/14.
//

#include <string>
#include <iostream>
#include <sstream>
#include "HttpResponse.hpp"
#include "status_code.hpp"

/*
 * nginx
 *  - Server
 *    - nginx
 *  - Date
 *    - Mon, 28 Sep 1970 06:00:00 GMT
 *  - Content-Type
 *    - charset=
 *  - Connection
 *    - upgrade
 *    - keep-alive
 *    - close
 *  - Content-Length
 *  - Content-Encoding
 *  - Location
 *    - https://  80:65535
 *    - http
 *  - Last-Modified
 *  - Accept-Ranges
 *  - Expires
 *  - Cache-Control
 *  - ETag
 *  - Transfer-Encoding
 *    - chunked
 *  - Keep-Alive
 *    - timeout=
 *  // GZIP
 *  - Vary
 *    - Accept-Encoding
 */

std::string GetResponseLine(int status_code) {
  switch (status_code) {
    case HTTP_OK:return "200 OK";
    case HTTP_CREATED:return "201 Created";
    case HTTP_ACCEPTED:return "202 Accepted";
    case HTTP_NO_CONTENT:return "204 No Content";
    case HTTP_PARTIAL_CONTENT:return "206 Partial Content";
    case HTTP_MOVED_PERMANENTLY:
      return "301 Moved Permanently";
    case HTTP_MOVED_TEMPORARILY:
      return "302 Moved Temporarily";
    case HTTP_SEE_OTHER:
      return "303 See Other";
    case HTTP_TEMPORARY_REDIRECT:
      return "307 Temporary Redirect";
    case HTTP_PERMANENT_REDIRECT:
      return "308 Permanent Redirect";
    case HTTP_BAD_REQUEST:
      return "400 Bad Request";
    case HTTP_UNAUTHORIZED:
      return "401 Unauthorized";
//    case HTTP_PAYMENT_REQUIRED:
    case HTTP_FORBIDDEN:
      return "403 Forbidden";
    case HTTP_NOT_FOUND:
      return "404 Not Found";
    case HTTP_NOT_ALLOWED:
      return "405 Not Allowed";
    case HTTP_REQUEST_TIME_OUT:
      return "408 Request Time-out";
    case HTTP_CONFLICT:
      return "409 Conflict";
//    case HTTP_GONE:
    case HTTP_LENGTH_REQUIRED:
      return "411 Length Required";
    case HTTP_PRECONDITION_FAILED:
      return "412 Precondition Failed";
    case HTTP_REQUEST_ENTITY_TOO_LARGE:
      return "413 Request Entity Too Large";
    case HTTP_REQUEST_URI_TOO_LARGE:
      return "414 Request-URI Too Large";
    case HTTP_UNSUPPORTED_MEDIA_TYPE:
      return "415 Unsupported Media Type";
    case HTTP_RANGE_NOT_SATISFIABLE:
      return "416 Requested Range Not Satisfiable";
    case HTTP_INTERNAL_SERVER_ERROR:
      return "500 Internal Server Error";
    case HTTP_NOT_IMPLEMENTED:
     return "501 Not Implemented";
    case HTTP_BAD_GATEWAY:
     return "502 Bad Gateway";
    case HTTP_SERVICE_UNAVAILABLE:
     return "503 Service Temporarily Unavailable";
    case HTTP_GATEWAY_TIME_OUT:
     return "504 Gateway Time-out";
    case HTTP_VERSION_NOT_SUPPORTED:
     return "505 HTTP Version Not Supported";
//    case HTTP_INSUFFICIENT_STORAGE:
//     return "507 Insufficient Storage";
    default:
      // TODO: ERROR
      return "ERROR NO OUTPUT";
  }
}

/**
 *
 * @return GMT time in RFC7231 format
 */
std::string CreateDate() {
  char date[1024];
  time_t gmt_time;

  time(&gmt_time);
  strftime(date, 1024, "%a, %d %b %Y %X %Z", gmtime(&gmt_time)); // RFC7231

  return std::string(date);
}

/**
 *
 * @param status_code
 * @return
 */
std::string CreateErrorSentence(int status_code) {
  std::stringstream error_sentence;

// Error file create
//  << "<!DOCTYPE html>" << CRLF;
//  << "<html><head>" << CRLF;
//  << "<title>Error</title></head>" << CRLF;
//  << "<body>Error</body>" << CRLF;

  error_sentence << "HTTP/1.1 " << GetResponseLine(status_code)
                 << CRLF; // TODO: check status code
  error_sentence << "Server: " << "42webserv" << "/1.0" << CRLF;
  error_sentence << "Date: " << CreateDate() << CRLF;
  error_sentence << "Content-Type: text/html" << CRLF;

  return error_sentence.str();
}

std::string CreateSimpleResponseBody(const std::string& response) {
  std::stringstream body;
  body << "<!DOCTYPE html>" << CRLF
    << "<html>" << CRLF
    << "<head>" << CRLF
    << "<title>" << response << "</title>" << CRLF
    << "</head>" << CRLF
    << "<body>" << CRLF
    << "<center><h1>" << response << "</h1></center>" << CRLF
    << "<hr><center>inception/0.0.1</center>" << CRLF
    << "</body>" << CRLF
    << "</html>" << CRLF;

  return (body.str());
}

std::string CreateSimpleResponse(int status_code) {
  std::stringstream response;

  std::string body = CreateSimpleResponseBody(GetResponseLine(status_code));

  response << CreateErrorSentence(status_code)
    << "content-length" << body.size() << CRLF
    << "connection:close" << CRLF << CRLF
    << body;

  return (response.str());
}
