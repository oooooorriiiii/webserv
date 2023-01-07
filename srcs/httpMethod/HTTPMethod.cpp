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
           const std::string &upload_file_path) {
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
    response_status = 204;
  } else {
    // file is new added
    // 201 content created
    if (errno == ENOENT) {
      response_status = 201;
    } else if (errno == EACCES) {
      response_message_str = CreateErrorResponse(403, err_pages);
      return (403); 
    } else {
      response_message_str = CreateErrorResponse(500, err_pages);
      return (500); 
		}
  } 

  // create file
  std::ofstream contents_file;
  std::ifstream from_file;
  std::stringstream   body;

  // try to open requested file
  contents_file.open(file_path.c_str());
  if (contents_file.is_open()) { // if all is well
    // check upload filepath
    ret_val = stat(upload_file_path.c_str(), &stat_buf);
    // if upload fille path exists, open and copy contents to requested file
    if (ret_val == 0 && S_ISREG(stat_buf.st_mode)) {
      from_file.open(upload_file_path.c_str());
      if (from_file.is_open()) {
        contents_file << from_file.rdbuf();
        // client expects body of new content if status code is 201
        from_file.seekg(0, from_file.beg);
        body << from_file.rdbuf();
      } else {
        response_message_str = CreateErrorResponse(500, err_pages);
        return (500); 
      }
    } else { // if upload filepath doesn't exist, use body from http request
      if (errno == ENOENT) {
        contents_file << http_body;
        body << http_body;
      } else if (errno == EACCES) {
        response_message_str = CreateErrorResponse(403, err_pages);
        return (403); 
      } else {
        response_message_str = CreateErrorResponse(500, err_pages);
        return (500); 
		  } 
    }
  } else {
    response_message_str = CreateErrorResponse(500, err_pages);
    return (500); 
  }
  from_file.close();
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

  return response_status;
}


/**
 *
 * @param http_header
 * @param file_path
 * @param response_message_str
 * @return
 */
/*
int do_get(std::string &response_message_str,
          const std::string &file_path,
          const ServerConfig::err_page_map &err_pages,
          const std::string &connection) {
  int response_status;
  std::stringstream response_message_stream;

  int ret_val;
  struct stat st = {};

  std::cout << "~~~~~~~~~~~~\n" << file_path << "\n~~~~~~~~~~~~" << std::endl;

  if (file_path[file_path.length() - 1] == '/') {
    response_message_stream << "HTTP/1.1 200 OK" << CRLF;
    response_status = 200;

    response_message_stream << "Server: " << "42webserv" << "/1.0" << CRLF;
    response_message_stream << "Date: " << CreateDate() << CRLF;
    response_message_stream << "Last-Modified: " << CreateDate() << CRLF;

    // index file does not exist
    std::string new_file_path = file_path;
    std::string::size_type pos = new_file_path.find("html//");
    if (pos != std::string::npos)
    {
      new_file_path.replace(pos, 6, "html/");
    }
    std::cout << "**********\n" << new_file_path << "\n**********" << std::endl;
    std::set<std::string> dirList = ft::CreateDirectoryList("./var/www/inception_server/html/");
    for (std::set<std::string>::iterator it = dirList.begin(); it != dirList.end(); it++) {
      response_message_stream << "\t\t" << *it << "<br>" << CRLF;
    }
    
    response_message_str = response_message_stream.str();
    return response_status;

  }

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

  std::cout << "**********\n" << file_path << "\n**********" << std::endl;
  
  // Normal URL -> index file exist

  // send body
  response_message_stream << CRLF;
  // file read
  std::ifstream reading_file;
  std::string reading_line_buf;
  reading_file.open(file_path.c_str());
  while (std::getline(reading_file, reading_line_buf))
    response_message_stream << reading_line_buf << '\n';
  

  response_message_str = response_message_stream.str();
  return response_status;
}
*/

int do_get(std::string &response_message_str,
          const std::string &file_path,
          const ServerConfig::err_page_map &err_pages,
          const std::string &connection) {
  int response_status;
  std::stringstream response_message_stream;

  int ret_val;
  struct stat st = {};

  std::cout << "**********\n" << file_path << "\n**********" << std::endl;
  std::cout << "~~~~~~~~~~~~~~~~~" << std::endl;
  ret_val = stat(file_path.c_str(), &st);
  if (ret_val < 0 || !S_ISREG(st.st_mode)) {
    response_message_str = CreateErrorResponse(404, err_pages);
    std::cout << "~~~~~~~~~~~~~~~~~" << std::endl;
    return (404);
  }


  std::cout << "**********\n" << file_path << "\n**********" << std::endl;


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
    response_message_stream << reading_line_buf << '\n';
  
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
  response_message_stream << "Connection: " << connection << CRLF << CRLF;

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

  std::cout << ":D ~~~~ throwing for: " << cgi.GetCgiSocket() << std::endl;
  // get contents by using poll
  throw ft::Socket::readCGIfd(cgi.GetCgiSocket());
}
