//
// Created by ymori on 2022/10/18.
//

#include "HTTPMethod.hpp"

#include <unistd.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>

#include "../httpResponse/HttpResponse.hpp"
#include "../HTTP/HTTPHead.hpp"
#include "../cgi/Cgi.hpp"
#include "../server/serverChild.hpp"


/*
 * The following implementations are omitted
 *
 * -- Check Mod, Match, Range and each processing
 * If-Modified-Since
 * If-Unmodified-Since
 * If-Match
 * If-None-Match
 * If-Range
 *
 * -- Check and judge status
 * 304
 * 412
 * 206
 * 200
 * 206
 * 200
 *
*/


/**
 *
 * @param http_body
 * @param file_path
 * @param response_message_str
 * @return
 */
int do_put(std::string &response_message_str,
           const std::string &file_path,
           const std::string &http_body,
           const std::string &connection) {
  int response_status;
  std::stringstream response_message_stream;

  // MUST

  struct stat stat_buf = {};
  int ret_val = stat(file_path.c_str(), &stat_buf);

  // MUST
  if (ret_val == 0) {
    // file is already exist
    // overwrite
    // 204 No Content
    response_message_stream << "HTTP/1.1 " << GetResponseLine(204) << CRLF;
    response_status = 204;
  } else {
    // file is new added
    // 201 content created
    response_message_stream << "HTTP/1.1 " << GetResponseLine(201) << CRLF;
    response_status = 201;
  }

  // create file
  std::ofstream contents_file;
  contents_file.open(file_path.c_str());
  contents_file << http_body;
  contents_file.close();

  response_message_stream << "Server: " << "42webserv" << "/1.0" << CRLF;
  response_message_stream << "Date: " << CreateDate() << CRLF;
  response_message_stream << "Connection: " << connection << CRLF;

  response_message_str = response_message_stream.str();

  return response_status;
}

/**
 *
 * @param http_header
 * @param file_path
 * @param response_message_str
 * @return
 */
int do_get(std::string &response_message_str,
          const std::string &file_path,
          const ServerConfig::err_page_map &err_pages,
          const std::string &connection) {
  int response_status;
  std::stringstream response_message_stream;

  int ret_val;
  struct stat st = {};

  ret_val = stat(file_path.c_str(), &st);
  if (ret_val < 0 || !S_ISREG(st.st_mode)) {
    response_message_str = CreateErrorResponse(404, err_pages);
    return (404);
  }

  response_message_stream << "HTTP/1.1 200 OK" << CRLF;
  response_status = 200;

  response_message_stream << "Server: " << "42webserv" << "/1.0" << CRLF;
  response_message_stream << "Date: " << CreateDate() << CRLF;
  response_message_stream << "Last-Modified: " << CreateDate() << CRLF;

//  // Force search results to text/html type
//  if (!file_path.empty()) {
//    // find type "x.html"
//  } else {
//    // find type file_path
//  }

  // sending partial content

  // send full entry
  // Content-Type:
  response_message_stream << "Content-Type: " << "text/html" << CRLF;
  // Content-Length:
  response_message_stream << "Content-Length: " << st.st_size << CRLF;
  response_message_stream << "Connection: " << connection << CRLF;

  // send body
  response_message_stream << CRLF;
  // file read
  std::ifstream reading_file;
  std::string reading_line_buf;
  reading_file.open(file_path.c_str());
  while (std::getline(reading_file, reading_line_buf))
    response_message_stream << reading_line_buf << CRLF;

  response_message_str = response_message_stream.str();
  return response_status;
}


/**
 *
 * @param http_header
 * @param file_path
 * @param response_message_str
 * @return
 */
int do_delete(std::string &response_message_str,
              const std::string &file_path,
              const ServerConfig::err_page_map &err_pages,
              const std::string &connection) {
  int response_status;
  std::stringstream response_message_stream;
  std::string delete_dir = "ok";

  // Range not allowed for DELETE
//  if (http_header.find("Range :") == http_header.end()) {
//    response_message_stream << CreateErrorSentence(501);
//    response_message_str = response_message_stream.str();
//    return 501;
//  }

  // unlink
  int ret = unlink(file_path.c_str());
  // TODO: check errno ?
  if (ret == 0) {
    response_message_stream << "HTTP/1.1 204 No Content" << CRLF;
    response_status = 204;
  } else {
    response_message_str = CreateErrorResponse(500, err_pages);
    return (500);
  }

  response_message_stream << "Server: " << "42webserv" << "/1.0" << CRLF;
  response_message_stream << "Date: " << CreateDate() << CRLF;
  response_message_stream << "Connection: " << connection << CRLF;

  response_message_str = response_message_stream.str();
  return response_status;
}


/**
 *
 * @param response_message_str
 * @return
 */
int do_CGI(std::string &response_message_str,
           ft::ServerChild server_child,
           std::string file_path,
           std::string query_string,
           const ServerConfig::err_page_map &err_pages) {
  std::stringstream response_message_stream;

  std::cout << "HELLO\n";
  /*
   * Check to exist file
   * FIXME: GETと同じことをしている．
   */
  int ret_val;
  struct stat st = {};

  ret_val = stat(file_path.c_str(), &st);
  if (ret_val < 0 || !S_ISREG(st.st_mode)) {
    response_message_str = CreateErrorResponse(404, err_pages);
    return (404);
  }

  /*
   * If the file exists, assign the file name to script_name_.
   */
  std::string script_name;
  std::size_t last_slash_pos = file_path.find_last_of('/');
  if (last_slash_pos != std::string::npos) {
    script_name = file_path.substr(last_slash_pos + 1);
  } else {
    script_name = file_path;
  }

  /*
   * Execute CGI
   */
  Cgi cgi(server_child, file_path, script_name, query_string);
  try {
    cgi.Execute();
  } catch (std::exception &e) {
    // status 500
    response_message_str = CreateErrorResponse(500, err_pages); 
    return (500);
  }

  // get contents from poll
  std::cout << ":D ~~~~ throwing for: " << cgi.GetCgiSocket() << std::endl;
  throw ft::Socket::readCGIfd(cgi.GetCgiSocket());
}
