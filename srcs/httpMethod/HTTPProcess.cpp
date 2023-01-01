//
// Created by ymori on 22/12/17.
//

#include <string>

#include "HTTPProcess.hpp"
#include "MethodUtils.hpp"

bool  is_cgi_dir(const std::string &plane_filepath , const std::string& kLocationAlias) {

  std::string cgi_dir = kLocationAlias + "/cgi-bin";
  bool is_CGI = false;

  if (plane_filepath.find(cgi_dir) != std::string::npos) {
    is_CGI = true;
  }

  return is_CGI;
}


/**
 *
 * @param server_child
 * @return
 *
 * @author ymori
 *
 */
std::string http_process(ft::ServerChild& server_child) {
  std::string response_message_str;

  /*
   * Get information from ServerChild
   */
  LocationConfig locationConf = server_child.Get_location_config();
  ft::HTTPHead httpHead = server_child.Get_HTTPHead();

  const std::string kRequestMethod = httpHead.GetRequestMethod();
  const std::string kFilePath = server_child.Get_path();
  const std::string kHttpBody = server_child.Get_body();
  ServerConfig::err_page_map err_pages = server_child.Get_server_config().getErrorPage();
  const std::string connection = get_connection(httpHead.GetHeaderFields());
  const std::string kLocationAlias = locationConf.getAlias();
  const std::string kLocationUri = locationConf.getUri();
  const std::string upload_file_path = locationConf.getUploadFilepath();
  const std::string upload_file = server_child.Get_path_parts(); // http://host:8080/locConf/<path_parts>


  /*
   * TODO: Check if CGI should be executed.
   * If URI contains CGI-path defined in the config file and the request method is POST or GET,
   * then execute CGI.
   */
  bool is_CGI = false;
  std::string query_string_;
  std::string plane_filepath = get_uri_and_check_CGI(kFilePath, query_string_, is_CGI);
  int ret;

  is_CGI = is_cgi_dir(plane_filepath, kLocationAlias);

  // TODO: delete. for debug
  std::cerr << "*************************" << std::endl;
  std::cerr << "server_child.Get_path(): " << kFilePath << std::endl;
  std::cerr << "plane_filepath:          " << plane_filepath << std::endl;
  std::cerr << "is_CGI:                  " << is_CGI << std::endl;
  std::cerr << "*************************" << std::endl;
  // ***
 
  if (kRequestMethod == "POST") {
    // Any POST request is CGI
    ret = do_CGI(response_message_str, server_child, plane_filepath,
                query_string_, err_pages);
  } else if (kRequestMethod == "GET") {
    if (is_CGI) {
      ret = do_CGI(response_message_str, server_child, plane_filepath,
                  query_string_, err_pages);
    } else {
      ret = do_get(response_message_str, plane_filepath, err_pages, connection); 
    }
  } else if (kRequestMethod == "PUT") {
    ret = do_put(response_message_str, plane_filepath, kHttpBody,
                err_pages, connection, upload_file_path + upload_file);
  } else if (kRequestMethod == "DELETE") {
    ret = do_delete(response_message_str, plane_filepath, err_pages, connection);
  } else {
    response_message_str = CreateErrorResponse(501, err_pages);
    ret = 501;
  }

  // set the status code to 400 so that socket will close the connection
  server_child.Set_response_code(connection == "close" ? 400 : ret);
  return response_message_str;
}

std::string get_connection(const http_header_t& headers) {
  http_header_t::const_iterator connection = headers.find("connection");

  if (connection == headers.end())
    return ("keep-alive");
  return (connection->second == "close" ? "close" : "keep-alive");
}