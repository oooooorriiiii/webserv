NAME := webserv
CXX := c++
CXXFLAGS := -std=c++98 -pedantic -Wall -Wextra -Werror
CONFIGSRCS := 	srcs/config/Config.cpp \
				srcs/config/ConfigParser.cpp \
				srcs/config/LocationConfig.cpp \
				srcs/config/ServerConfig.cpp
HTTPSRCS := srcs/HTTP/HTTPHead.cpp
SOCKETSRCS := 	srcs/server/server.cpp \
				srcs/server/serverChild.cpp \
				srcs/server/socket.cpp
HTTPMETHODSRCS :=	srcs/cgi/Cgi.cpp \
					srcs/httpMethod/AutoIndex.cpp \
					srcs/httpMethod/HTTPMethod.cpp \
					srcs/httpMethod/HTTPProcess.cpp \
					srcs/httpMethod/MethodUtils.cpp
HTTPRESPONSESRCS :=	srcs/httpResponse/HttpResponse.cpp
SRCS := main.cpp srcs/utils/utils.cpp $(CONFIGSRCS) $(HTTPSRCS) $(SOCKETSRCS) $(HTTPMETHODSRCS) $(HTTPRESPONSESRCS)
OBJS := $(SRCS:.cpp=.o)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@if ! grep "127.0.0.1 webserv" /etc/hosts >/dev/null; then \
		sudo -- sh -c "echo 127.0.0.1 webserv >> /etc/hosts"; \
	fi;
	@if ! grep "127.0.0.1 first" /etc/hosts >/dev/null; then \
		sudo -- sh -c "echo 127.0.0.1 first >> /etc/hosts"; \
	fi;
	@if ! grep "127.0.0.1 second" /etc/hosts >/dev/null; then \
		sudo -- sh -c "echo 127.0.0.1 second >> /etc/hosts"; \
	fi;
	@if ! grep "127.0.0.1 other" /etc/hosts >/dev/null; then \
		sudo -- sh -c "echo 127.0.0.1 other >> /etc/hosts"; \
	fi;

all: $(NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)
	@if grep "127.0.0.1 webserv" /etc/hosts >/dev/null; then \
		sudo -- sh -c "sed -ie \"/127.0.0.1 webserv/d\" /etc/hosts"; \
	fi;
	@if grep "127.0.0.1 first" /etc/hosts >/dev/null; then \
		sudo -- sh -c "sed -ie \"/127.0.0.1 first/d\" /etc/hosts"; \
	fi;
	@if grep "127.0.0.1 second" /etc/hosts >/dev/null; then \
		sudo -- sh -c "sed -ie \"/127.0.0.1 second/d\" /etc/hosts"; \
	fi;
	@if grep "127.0.0.1 other" /etc/hosts >/dev/null; then \
		sudo -- sh -c "sed -ie \"/127.0.0.1 other/d\" /etc/hosts"; \
	fi;

re: fclean all

.PHONY: all clean fclean re