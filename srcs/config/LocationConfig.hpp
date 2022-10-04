#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include "ConfigConstant.hpp"

class LocationConfig
{
private:
	std::set<E_DirectibeType> is_set;

	std::string uri;
	std::string alias;
	bool autoindex;
	std::vector<std::string> allow_method;
	std::vector<std::string> index;
	std::vector<std::string> cgi_extension;
	std::map<int, std::string> redirect;
	std::map<int, std::string> error_page;
	std::string upload_filepath;

public:
	LocationConfig();
	~LocationConfig();

	void setUri(const std::string &uri);
	void setAlias(const std::string &alias);
	void setAutoindex(const bool autoindex);
	void addAllowMethod(const std::string &allow_method);
	void addIndex(const std::string &index);
	void addCgiExtension(const std::string &cgi_extension);
	void addRedirect(const int redirect_status, const std::string &uri);
	void addErrorPage(const int error_status, const std::string &uri);
	void setUploadFilepath(const std::string &upload_filepath);

	const std::string &getUri() const;
	const std::string &getAlias() const;
	const bool &getAutoIndex() const;
	const std::vector<std::string> &getAllowMethod() const;
	const std::vector<std::string> &getIndex() const;
	const std::vector<std::string> &getCgiExtension() const;
	const std::map<int, std::string> &getRedirect() const;
	const std::map<int, std::string> &getErrorPage() const;
	const std::string &getUploadFilepath() const;

	const bool &isSet(E_DirectibeType type);
};

#endif