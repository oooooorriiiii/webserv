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
           const ServerConfig::err_page_map &err_pages,
           const std::string &connection,
           const std::string &upload_file_path,
           const std::set<std::string> &allow_method) {
  int response_status;
  std::stringstream response_message_stream;

  // MUST

  struct stat stat_buf = {};

  std::ifstream from_file;
  std::stringstream   body;

  // check if upload_filepath exists
  int ret_val = stat(upload_file_path.c_str(), &stat_buf);

  // if upload file path exists, open and stream contents to body
  if (ret_val == 0 && S_ISREG(stat_buf.st_mode)) {
    from_file.open(upload_file_path.c_str());
    if (from_file.is_open()) {
      body << from_file.rdbuf();
      from_file.close();
    } else {
      response_message_str = CreateErrorResponse(500, err_pages, allow_method);
      return (500);
    }
  } else { // if upload filepath doesn't exist, use body from http request
    if (errno == ENOENT) {
      body << http_body;
    } else if (errno == EACCES) {
      response_message_str = CreateErrorResponse(403, err_pages, allow_method);
      return (403); 
    } else {
      response_message_str = CreateErrorResponse(500, err_pages, allow_method);
      return (500); 
		} 
  }

  ret_val = stat(file_path.c_str(), &stat_buf);

  // MUST
  if (ret_val == 0) {
    // file is already exist
    // overwrite
    // 204 No Content
    response_status = 204;
  } else {
    // file is new added
    // 201 content created
    if (errno == ENOENT) {
      response_status = 201;
    } else if (errno == EACCES) {
      response_message_str = CreateErrorResponse(403, err_pages, allow_method);
      return (403); 
    } else {
      response_message_str = CreateErrorResponse(500, err_pages, allow_method);
      return (500); 
		}
  } 

  // create file
  std::ofstream contents_file;

  // try to open requested file
  contents_file.open(file_path.c_str());
  if (contents_file.is_open()) { // if all is well
    contents_file << body.str(); 
  } else {
    response_message_str = CreateErrorResponse(500, err_pages, allow_method);
    return (500); 
  }
  contents_file.close();
  
  response_message_stream << "HTTP/1.1 " << GetResponseLine(response_status) << CRLF;
  response_message_stream << "Server: " << "42webserv" << "/1.0" << CRLF;
  response_message_stream << "Date: " << CreateDate() << CRLF;
  response_message_stream << "Connection: " << connection << CRLF;

  if (response_status == 201) { // send content-length header and body for 201
    response_message_stream << "Content-length: " << body.str().size() << CRLF << CRLF
                            << body.str();
  } else {
    response_message_stream << CRLF;
  }

  response_message_str = response_message_stream.str();

  return (connection == "close" ? 400 : response_status);
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
          const std::string &connection,
          bool autoindex,
          const std::string alias,
          const std::set<std::string> &allow_method) {
  int response_status;
  std::stringstream response_message_stream;
  std::stringstream body_stream;
  std::string       body;

  int ret_val;
  struct stat st = {};

  ret_val = stat(file_path.c_str(), &st);
  if (ret_val == 0) {
    if (S_ISREG(st.st_mode)) {
      // file read
      std::ifstream reading_file;
      std::string reading_line_buf;
      reading_file.open(file_path.c_str());
      while (std::getline(reading_file, reading_line_buf))
        body_stream << reading_line_buf << '\n';
      body = body_stream.str();
    } else if (S_ISDIR(st.st_mode) && autoindex) {
      std::string tmpFilePath = file_path;
      // when file path not end "/", do redirect to "PATH/"
      if (tmpFilePath[tmpFilePath.size() - 1] != '/') {
        tmpFilePath = tmpFilePath + "/";
        std::string body = CreateSimpleResponseBody(GetResponseLine(302));
        std::size_t i = 0;
        int flag = 0;
        while (i < tmpFilePath.length() && flag == 0) {
          if (tmpFilePath[i] != alias[i])
            flag = 1;
          i++;
        }
        std::string autoIndexRedirectPath = tmpFilePath.substr(i);
        response_message_str = CreateRedirectResponse(302, autoIndexRedirectPath);
        return (302);
      }
      std::string autoIndexFilePath = tmpFilePath.substr(tmpFilePath.find_last_of("/"));
      std::set<std::string> dirList = ft::CreateDirectoryList(tmpFilePath);
      std::size_t i = 0;
      int flag = 0;
      while (i < tmpFilePath.length() && flag == 0) {
        if (tmpFilePath[i] != alias[i])
          flag = 1;
        i++;
      }
      std::string autoIndexTitlePath = tmpFilePath.substr(i);
      body_stream << "<head>" << CRLF
        << "<meta charset='utf-8'>" << CRLF
        << "</head>" << CRLF;
      body_stream << "<h1>Index of /" << autoIndexTitlePath << "</h1>" << CRLF;
      body_stream << "<hr>" << CRLF;
      for (std::set<std::string>::iterator it = dirList.begin(); it != dirList.end(); it++) {
        body_stream << "\t\t<a href='." << autoIndexFilePath << *it << "'>" << *it << "</a><br>" << CRLF;
      }
      body = body_stream.str();
    } else {
      response_message_str = CreateErrorResponse(403, err_pages, allow_method);
      return (403); 
    }
  } else {
    if (errno == ENOENT) {
      response_message_str = CreateErrorResponse(404, err_pages, allow_method);
      return (404);
    } else if (errno == EACCES) {
      response_message_str = CreateErrorResponse(403, err_pages, allow_method);
      return (403); 
    } else {
      response_message_str = CreateErrorResponse(500, err_pages, allow_method);
      return (500); 
		}
  }

  response_message_stream << "HTTP/1.1 200 OK" << CRLF;
  response_status = connection == "close" ? 400 : 200;

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
  response_message_stream << "Content-Length: " << body.size() << CRLF;
  response_message_stream << "Connection: " << connection << CRLF;

  // send body
  response_message_stream << CRLF;
  response_message_stream << body;

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
              const std::string &connection,
              const std::set<std::string> &allow_method) {
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
    response_message_str = CreateErrorResponse(500, err_pages, allow_method);
    return (500);
  }

  response_message_stream << "Server: " << "42webserv" << "/1.0" << CRLF;
  response_message_stream << "Date: " << CreateDate() << CRLF;
  response_message_stream << "Connection: " << connection << CRLF << CRLF;

  response_message_str = response_message_stream.str();
  return connection == "close" ? 400 : response_status;
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
           const ServerConfig::err_page_map &err_pages,
           const std::set<std::string> &allow_method) {
  std::stringstream response_message_stream;

  /*
   * Check to exist file
   * FIXME: GETと同じことをしている．
   */
  int ret_val;
  struct stat st = {};

  ret_val = stat(file_path.c_str(), &st);
  if (ret_val < 0 || !S_ISREG(st.st_mode)) {
    response_message_str = CreateErrorResponse(404, err_pages, allow_method);
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
    response_message_str = CreateErrorResponse(500, err_pages, allow_method);
    return (500);
  }

  std::cout << ":D ~~~~ throwing for: " << cgi.GetCgiSocket() << std::endl;
  // get contents by using poll
  throw ft::Socket::readCGIfd(cgi.GetCgiSocket());
}
