#include "srcs/socket/server.hpp"

int main()
{
	ft::Server server("config/hoge.conf");

	server.start_server();
	return 0;
}
