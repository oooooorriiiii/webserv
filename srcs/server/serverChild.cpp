#include "serverChild.hpp"
#include "../httpMethod/MethodUtils.hpp"
#include <iostream>
namespace ft
{

	ServerChild::ServerChild(): server_config_(ServerConfig()), location_config_(),
		redirectList_map_(), response_code_(), parse_status_(), HTTP_head_(),
		content_length_(), read_bytes_(), max_body_size_(), body_(), save_(),
		path_(), path_parts_(), hex_bytes_()
	{
	}

	ServerChild::~ServerChild()
	{
	}

	ServerChild::ServerChild(const ServerConfig &server_config)
		: server_config_(server_config), location_config_(), redirectList_map_(),
		response_code_(), parse_status_(), HTTP_head_(), content_length_(),
		read_bytes_(), max_body_size_(), body_(), save_(), path_(), path_parts_(),
		hex_bytes_()
	{
		ServerConfig::loc_conf_map::const_iterator itend = server_config.getLocationConfigList().end();
		for (ServerConfig::loc_conf_map::const_iterator it = server_config.getLocationConfigList().begin(); it != itend; ++it)
		{
			LocationConfig location_config = (*it).second;
			if (location_config.getRedirect().first != LocationConfig::NO_REDIRECT)
			{ // LocationConfig にredirectが設定されている
				std::pair<int, const std::string> tmp = location_config.getRedirect();
				const std::string &redirect_uri = location_config.getUri();
				redirectList_map_[redirect_uri].uri = redirect_uri;
				redirectList_map_[redirect_uri].status_code = tmp.first;
				redirectList_map_[redirect_uri].dest_uri = tmp.second;
			}
		}
	}

	ServerChild::ServerChild(const ServerChild &src) {
		server_config_ = src.server_config_;
		location_config_ = src.location_config_;	
		redirectList_map_ = src.redirectList_map_;
		response_code_ = src.response_code_;
		parse_status_ = src.parse_status_;
		HTTP_head_ = src.HTTP_head_;
		content_length_ = src.content_length_;
		read_bytes_ = src.read_bytes_;
		max_body_size_ = src.max_body_size_;
		body_ = src.body_;
		save_ = src.save_;
		path_ = src.path_;
		path_parts_ = src.path_parts_;
		hex_bytes_ = src.hex_bytes_;
	}

	ServerChild& ServerChild::operator=(const ServerChild &rhs){
		if (this != &rhs) {
			HTTP_head_ = rhs.HTTP_head_;
			server_config_ = rhs.server_config_;
			location_config_ = rhs.location_config_;	
			redirectList_map_ = rhs.redirectList_map_;
			response_code_ = rhs.response_code_;
			parse_status_ = rhs.parse_status_;
			content_length_ = rhs.content_length_;
			read_bytes_ = rhs.read_bytes_;
			max_body_size_ = rhs.max_body_size_;
			body_ = rhs.body_;
			save_ = rhs.save_;
			path_ = rhs.path_;	
			path_parts_ = rhs.path_parts_;
			hex_bytes_ = rhs.hex_bytes_;
		}
		return (*this);
	}

	bool ServerChild::is_redirect_(const std::string &url)
	{

		// std::cout <<"url" <<url << std::endl;
		// std::cout <<"url.dest" << redirectList_map_[url].dest_uri << std::endl;
		// std::cout << redirectList_map_["/redirect/"].dest_uri << std::endl;
		redir_map::const_iterator it = redirectList_map_.upper_bound(url);
		if (it == redirectList_map_.begin())
			return (false);
		--it;
		if ((*it).second.uri.compare(0, (*it).second.uri.size(), url, 0, (*it).second.uri.size()) == 0)
			return (true);
		return (false);
	}

	int		ServerChild::Get_response_code() const { return response_code_; }
	const HTTPParseStatus&	ServerChild::Get_parse_status() const { return parse_status_;}
	HTTPHead&			ServerChild::Get_HTTPHead() { return HTTP_head_; }
	const std::string&		ServerChild::Get_body() const { return body_; }
	const std::string&		ServerChild::Get_path() const { return path_; }
	const std::string&		ServerChild::Get_path_parts() const { return path_parts_; }
	LocationConfig&		ServerChild::Get_location_config() { return location_config_; }
	ServerConfig&		ServerChild::Get_server_config() { return server_config_; }

	void	ServerChild::Set_parse_status(HTTPParseStatus parse_status) { parse_status_ = parse_status; }
	void	ServerChild::Set_response_code(int response_code) { response_code_ = response_code; }

	void	ServerChild::SetUp(HTTPHead& head) {
		// set up httRequest head
		HTTP_head_ = head;
		save_ = HTTP_head_.getSave();
		max_body_size_ = server_config_.getClientMaxBodySize();
		response_code_ = head.GetResponseCode();

		// validate good request headers
		check_headers_();

		// Find location conf
		setUp_locationConfig_();

		// validate request method
		check_method_();

		// return in case of redirect
		if (parse_status_ == complete) {
			return ;
		}

		// use index from config if URI is a directory
		attach_index();

		// decide parse status and get content-length if needed
		decide_parse_status_();

		if (parse_status_ == readStraight) {
			header_field_map::iterator content_length = HTTP_head_.GetHeaderFields().find("content-length");

          	content_length_ = strBase_to_UI_(content_length->second, std::dec);
           	if (content_length_ > max_body_size_) {
               	throw_(413, "Payload Too Large");
           	}
           	read_bytes_ = content_length_;
		}
	}
	
	void	ServerChild::Parse(const std::string& content) {
		save_ += content;
        if (parse_status_ == readChunks && !save_.empty()) {
            read_chunks_();
        } else if (parse_status_ == readStraight) {
            read_straight_();
        }
	}

    unsigned int     ServerChild::strBase_to_UI_(const std::string& str, std::ios_base& (*base)(std::ios_base&)) {
        std::stringstream  stream(str);
        unsigned long int  num;

        stream >> base >> num;

        if (num > std::numeric_limits<unsigned int>::max()) {
            throw_(400, "Bad Request - invalid length");
        }
        return (static_cast<unsigned int>(num));
    }

	void    ServerChild::throw_(int responseCode, const std::string& message) {
        response_code_ = responseCode;
        throw std::runtime_error(message);
    }

	void    ServerChild::setUp_locationConfig_() {
        std::string     httpReqURI = HTTP_head_.GetRequestURI();
        ServerConfig::loc_conf_map				serverLocMap = server_config_.getLocationConfigList();
        ServerConfig::loc_conf_map::iterator	locConfIt;

        while ((locConfIt = serverLocMap.find(httpReqURI)) == serverLocMap.end() && !httpReqURI.empty()) {
            size_t i =  httpReqURI.find_last_of('/');
            path_parts_.insert(0, httpReqURI.substr(i)); // from index to npos
            httpReqURI.erase(i); // from index to npos
        }
 
        if (locConfIt == serverLocMap.end()) {
			std::cout << "could not find LocationConfig, using default\n";
			locConfIt = serverLocMap.find("/");
        } else {
			std::cout << "found location config: " + locConfIt->first << std::endl;;
		}

        location_config_ = locConfIt->second;
		if (location_config_.getRedirect().first != LocationConfig::NO_REDIRECT) {
			if (HTTP_head_.GetRequestMethod() != "GET") {
				throw_(501, "Redirect only allowed with GET");
			}
			parse_status_ = complete;
			response_code_ = location_config_.getRedirect().first;
			path_ = location_config_.getRedirect().second;
		} else {
	        path_ = location_config_.getAlias() + location_config_.getUri() + path_parts_;
		}
    }

	void	ServerChild::check_method_ () {
		std::set<std::string> valid_methods = location_config_.getAllowMethod();
		std::set<std::string>::iterator end = valid_methods.end();
		std::string method = HTTP_head_.GetRequestMethod();

		std::cout << "REQUEST METHOD: " << HTTP_head_.GetRequestMethod() << std::endl << " ALLOWED METHODS: ";
		for (std::set<std::string>::iterator it = valid_methods.begin(); it != end; ++it)
			std::cout << *it << " " << std::endl;
		std::cout << std::endl;
		if (valid_methods.find(method) == end) {
			// 405 if known method is not in locConf 'allowed_method'
			if (method == "PUT" || method == "GET" || method == "DELETE" || method == "POST")
				throw_(405, "Method Not allowed");
			// 501 if method is completely unkown
            throw_(501, "Not Implemented - invalid request method");
        }
	}

	void	ServerChild::attach_index() {
		std::vector<std::string> indexes = location_config_.getIndex();
		bool needExistingFile = 
			HTTP_head_.GetRequestMethod() == "GET"
			|| HTTP_head_.GetRequestMethod() == "DELETE" ?
				true : false;
		struct stat sb;

        /*
         * Get plane file path
         */
        std::string plane_uri;
        std::string tmp_query;
        bool        tmp_is_cgi;
        plane_uri = get_uri_and_check_CGI(path_, tmp_query, tmp_is_cgi);

        if (stat(plane_uri.c_str(), &sb) == -1) {
			std::cout << "ERNO: " << errno << std::endl;
			if (errno == EACCES)
				throw_(403, "forbidden - cannot access uri path");
			else if (errno == ENOENT) {
				if (needExistingFile)// PUT & POST can be ENOENT
					throw_(404, "NotFound - path doesn't exist");
			}
			else {
				throw_(500, "Internal error - nomem or other");
			}
		}
		if (!S_ISDIR(sb.st_mode))
			return ;	

		for (std::vector<std::string>::iterator it = indexes.begin(); it != indexes.end(); ++it) {
			std::string tmp = path_
							+ (path_[path_.size() - 1] == '/' || *((*it).begin()) == '/' ? "" : "/")
							+ *it;
			if (stat(tmp.c_str(), &sb) == -1 || !S_ISREG(sb.st_mode)) {
				continue ;	
			}
			path_parts_ = *it;
			path_ = tmp;
			break ;
		}
	}

	void	ServerChild::check_headers_() {
		header_field_map headers = HTTP_head_.GetHeaderFields();
		for (header_field_map::iterator it = headers.begin(); it != headers.end(); ++it) {
			// check for WSP in key
        	if (isspace(it->first[it->first.size() - 1])) {
            	throw_(400, "Bad Request - trailing white space after header key");
			}
        	// Check for obs-fold in value
        	if (it->second[it->second.size() - 1] == '\\') {
            	throw_(400, "Bad Request - obs-fold in header value");
        	}
			// Check for Trailer
			if (it->first == "trailer") {
				throw_(501, "Not Implemented - Trailer included");
			}
			// check transfer-encoding
			if (it->first == "transfer-encoding" && it->second != "chunked") {
                throw_(501, "Not Implemented - does not understand transfer-encoding");
            }
			// check and set content-length and read_bytes
			if (it->first == "content-length" && it->second.find(',') != std::string::npos) {
                throw_(400, "Bad Request - comma separated content-length");
			}
		}
	}

	void	ServerChild::decide_parse_status_() {
		header_field_map headers = HTTP_head_.GetHeaderFields();
		header_field_map::iterator transfer_encoding = headers.find("transfer-encoding");
        header_field_map::iterator content_length = headers.find("content-length");

        if (transfer_encoding != headers.end() && content_length != headers.end()) {
            headers.erase(content_length); 
            throw_(400, "Bad Request - both transfer-encoding and content-length exist");
        } else if (transfer_encoding != headers.end()) {
            parse_status_ = readChunks;
        } else if (content_length!= headers.end()) { 
            parse_status_ = readStraight;
        } else {
            if (HTTP_head_.GetRequestMethod() == "POST") {
                throw_(400, "Bad Request - POST expects content-length or transfer-encoding");
            }
            response_code_ = 200;
            parse_status_ = complete;
        }
	}

	void	ServerChild::read_body_(unsigned int len) {
 	    read_bytes_ -= len;
    	body_ += save_.substr(0, len);
     	save_.erase(0, len);	
	}

    void	ServerChild::read_straight_() {
		if (read_bytes_ >= save_.size()) {
			read_body_(save_.size());
		} else {
            body_ += save_.substr(0, read_bytes_);
            save_.erase(0, read_bytes_);
            read_bytes_ = 0;
            throw_(400, "Bad Request - unexpected body bytes(straight)");
        }	
        if (read_bytes_ == 0) {
            response_code_ = 200;
            parse_status_ = complete;
        }
	}

    void	ServerChild::read_chunks_() {
		// if read_bytes == 0, remove leading delim if it exists, and reset hex_bytes to 0
		if (hex_bytes_ && !read_bytes_) {
			if (save_.find(DELIM) == 0) {
				save_.erase(0, DELIM.size());
			}
			hex_bytes_ = 0;
			if (save_.empty())
				return;
		}	
		// get hex bytes if == 0
		if (!hex_bytes_){
			get_hex_read_bytes_();
			hex_bytes_ = read_bytes_;
		}
        while (hex_bytes_ && !save_.empty()) {
			// if save_ is smaller/equal to readbytes, read all bytes from save into body 
			if (read_bytes_ >= save_.size() - DELIM.size()) {
				if (read_bytes_ == save_.size() - DELIM.size()) { // if save includes delim (not for reading), remove it
					if (save_.substr(read_bytes_) != DELIM) { // if last bytes not == delim, chunk is larger than hex
						throw_(400, "Bad Request - unexpected body bytes DELIM doesn't match");
					}
					save_.erase(read_bytes_, DELIM.size());
				}
				read_body_(save_.size());
			} else { // if save_ is larger than remaining readbytes read remaining readbytes from save into body
				read_body_(read_bytes_);
				if (save_.find(DELIM) != 0) { // if delim is not first, there are unexpected body bytes
		            throw_(400, "Bad Request - unexpected body bytes (chunked)");
				}
				save_.erase(0, DELIM.size()); // remove delim
			}
			// if no save left or no delim, return. must retrive more string for reading, or next hex
			if (save_.empty() || (save_.find(DELIM) == std::string::npos)) {
				break;
			}
			// get next hex 
			if (read_bytes_ == 0) {
				get_hex_read_bytes_();
				hex_bytes_ = read_bytes_;
			}
        }
	    if (hex_bytes_ == 0) {	
			response_code_ = 200;
			parse_status_ = complete;
		}
	}

	void	ServerChild::get_hex_read_bytes_() {
        size_t i = save_.find(DELIM);

        read_bytes_ = strBase_to_UI_(save_.substr(0, i), std::hex);
		save_.erase(0, i + DELIM.size());

        if (read_bytes_ > max_body_size_) {
            throw_(413, "Payload Too Large");
		}
	}
} // namespace ft
