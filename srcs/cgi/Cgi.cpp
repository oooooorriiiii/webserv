//
// Created by yuumo on 2022/11/07.
//

/*
 * According to RFC 3875, CGI responses can be document-response, local-redir-response, client-redir-response, client-redirdoc-response, but our implementation is limited to document-response.
 */


#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <vector>
#include <cstring>
#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <sys/socket.h>
#include <cstdio>
#include <sys/wait.h>
#include <sys/time.h>
#include "Cgi.hpp"

Cgi::Cgi(ft::ServerChild server_child,
         const std::string &file_path,
         const std::string &script_name,
         const std::string &query_string)
    : cgi_path_(file_path),
      query_string_(query_string),
      content_length_(server_child.Get_body().size()), // The order is reversed, but that's OK.
      request_method_(server_child.Get_HTTPHead().GetRequestMethod()),
      script_name_(script_name),
      cgi_extension_(server_child.Get_location_config().getCgiExtension().first),
      bin_path_(server_child.Get_location_config().getCgiExtension().second),
      server_name_(server_child.Get_server_config().getServerName()),
      server_port_(server_child.Get_server_config().getListen())
      {
}

Cgi::~Cgi() { 
  // the cgi_socket will be close in
  // Socket::recieve_msg_from_cgi_()
}

/**
 *
 * @param request_method
 *
 * TODO: We must build this function
 * - set environment
 *
 * https://datatracker.ietf.org/doc/html/rfc3875#section-4-1
 * http://bashhp.web.fc2.com/WWW/header.html
 */
void Cgi::CreateEnvMap() {
  cgi_env_val_["SERVER_SOFTWARE"] = "42";
  cgi_env_val_["GATEWAY_INTERFACE"] = "CGI/1.1";
  cgi_env_val_["SERVER_PROTOCOL"] = "HTTP/1.1"; // tmp

  std::stringstream server_port_string_; // server_port_ is unsigned int.
  server_port_string_ << server_port_;
  cgi_env_val_["SERVER_PORT"] = server_port_string_.str();

  cgi_env_val_["SERVER_NAME"] = server_name_;
  cgi_env_val_["REQUEST_METHOD"] = request_method_;
  cgi_env_val_["SCRIPT_NAME"] = script_name_;

  // POST ????????????????????????????????????????????????????????????????????????????????????????????????byte???
  // Therefore, for the GET method, CONTENT_LENGTH is 0.
  std::stringstream content_length_string_;
  content_length_string_ << content_length_;
  cgi_env_val_["CONTENT_LENGTH"] = content_length_string_.str();

  // POST ?????????????????????????????????????????????
  cgi_env_val_["CONTENT_TYPE"] = "html"; // tmp

  // for GET
  cgi_env_val_["QUERY_STRING"] = query_string_;
  cgi_env_val_["PATH_INFO"] = "";

  cgi_env_val_["REQUEST_URI"] = ""; // Not supported
}

/**
 * @brief set environ based on cgi_env_val_
 */
void  Cgi::SetEnv() {
  for (std::map<std::string, std::string>::const_iterator iter = cgi_env_val_.begin(); iter != cgi_env_val_.end(); ++iter) {
    setenv(iter->first.c_str(), iter->second.c_str(), 1); // overwrite
  }
}

/**
 * @param from
 * @param to
 * @return
 */
int Cgi::change_fd(int from, int to) {
  int ret_val;

  if (from == to) {
    return 0;
  }
  close(to);
  ret_val = dup2(from, to);
  return ret_val;
}

void  cgi_timeout_handler(int signum) {
  (void)signum;

  exit(1);
}

/**
 * @brief Execute CGI.
 *
 * error:
 * ???????????????????????????????????????
 * ???????????????exit????????????
 *
 * CGI????????????????????????????????????
 */
void Cgi::Execute() {
  int ret_val;
  int socket_fds[2];

  ret_val = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
  if (ret_val == -1) {
    throw std::runtime_error("CGI error: socket error");
  }

  int parent_socket = socket_fds[0];
  int child_socket = socket_fds[1];


  /*
   * Create argv
   */
  char **argv = (char **)malloc(sizeof(char *) * 3);
  if (argv == NULL) {
    throw std::runtime_error("CGI error: allocation failed");
  }
  argv[0] = strdup(bin_path_.c_str());
  if (argv[0] == NULL) {
    free(argv);
    throw std::runtime_error("CGI error: allocation failed");
  }
  argv[1] = strdup(cgi_path_.c_str());
  if (argv[1] == NULL) {
    free(argv[0]);
    free(argv);
    throw std::runtime_error("CGI error: allocation failed");
  }
  argv[2] = NULL;

  /*
   * Create child process
   */
  pid_t pid = fork();
  if (pid < 0) { // fork error
    close(parent_socket);
    close(child_socket);
    free(argv[0]);
    free(argv[1]);
    free(argv);
    throw std::runtime_error("CGI error: fork failed");
  }
  if (pid == 0) { // child
    int ret_val_child = 1;

    Cgi::CreateEnvMap();
    Cgi::SetEnv();

    close(parent_socket);

    /*
     * Set timeout
     */
    struct itimerval itimerval = {};
    itimerval.it_value.tv_sec = 5; // timeout sec
    itimerval.it_value.tv_usec = 0;
    itimerval.it_interval.tv_sec = 0;
    itimerval.it_interval.tv_usec = 0;
    signal(SIGALRM, cgi_timeout_handler);
    setitimer(ITIMER_REAL, &itimerval, NULL);

    /*
     * Connect STDIN and STDOUT to the file descriptor of a socket.
     */
    ret_val_child = change_fd(child_socket, STDIN_FILENO);
    if (ret_val_child < 0) {
      exit(ret_val_child);
    }
    ret_val_child = change_fd(child_socket, STDOUT_FILENO);
    if (ret_val_child < 0) {
      exit(ret_val_child);
    }

    /*
     * Change directory
     */
    ret_val_child = chdir("./");
    if (ret_val_child != 0) {
      exit(ret_val_child);
    }


    /*
     * Execution CGI
     */
    errno = 0;
    ret_val_child = execve(bin_path_.c_str(), argv, environ);
    perror("execve failed");

    exit(ret_val_child);
  }

  /*
   * free argv for execve
   */
  free(argv[0]);
  free(argv[1]);
  free(argv);

  // Set cgi socket
  cgi_socket_ = parent_socket;

  int status;
  waitpid(pid, &status, 0);

  if (WEXITSTATUS(status) != 0 || WIFSIGNALED(status)) {
    std::cerr << "The child process exited with an error" << std::endl;
    close(child_socket);
    close(parent_socket);
    throw std::runtime_error("CGI process failed");
  }
// if to output log
  else
  {
    std::cerr << "The child process exited successfully" << std::endl;
  }

  close(child_socket);
}

/*
 * Getter
 */
int Cgi::GetCgiSocket() const {
  return cgi_socket_;
}
