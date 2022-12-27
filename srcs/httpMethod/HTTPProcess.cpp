//
// Created by ymori on 22/12/17.
//

#include <string>

#include "HTTPMethod.hpp"
#include "MethodUtils.hpp"

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
  const std::string kRequestMethod = server_child.Get_HTTPHead().GetRequestMethod();
  const std::string kFilePath = server_child.Get_path();
  const std::string kHttpBody = server_child.Get_body();

  /*
   * TODO: Check if CGI should be executed.
   * If URI contains CGI-path defined in the config file and the request method is POST or GET,
   * then execute CGI.
   */
  bool is_CGI = false;
  std::string query_string_;
  std::string plane_filepath = get_uri_and_check_CGI(kFilePath, query_string_, is_CGI);
  int ret;

  // TODO: delete. for debug
  std::cerr << "*************************" << std::endl;
  std::cerr << "server_child.Get_path(): " << kFilePath << std::endl;
  std::cerr << "plane_filepath:          " << plane_filepath << std::endl;
  std::cerr << "*************************" << std::endl;
  // ***

  if (kRequestMethod == "POST") {
    // Any POST request is CGI
    ret = do_CGI(response_message_str, server_child, plane_filepath, query_string_);
    server_child.Set_response_code(ret);
  } else if (kRequestMethod == "GET") {
    if (is_CGI) {
      ret = do_CGI(response_message_str, server_child, plane_filepath, query_string_);
    } else {
      ret = do_get(response_message_str, plane_filepath);
    }
    server_child.Set_response_code(ret);
  } else if (kRequestMethod == "PUT") {
    ret = do_put(response_message_str, plane_filepath, kHttpBody);
    server_child.Set_response_code(ret);
  } else if (kRequestMethod == "DELETE") {
    ret = do_delete(response_message_str, plane_filepath);
    server_child.Set_response_code(ret);
  } else {
    ret = disallow_method(response_message_str);
    server_child.Set_response_code(ret);
  }

  return response_message_str;
}
