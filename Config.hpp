#ifndef	CONFIG_HPP
#define	CONFIG_HPP

#include <string>
#include <vector>
#include <set>
#include <map>

struct Redirect
{
	int			status;			// 0 = pas de redir
	std::string	target;			// comme "/picture.img" ou "http://..."
	Redirect() : status(0) {}
};

struct LocationConfig
{
	std::string					path;			// comme "/picture.img" ou "http://..."
	std::string					root;
	std::vector<std::string>	index_files;	
	std::set<std::string>		methods;		// {"GET", "POST", "DELETE"}
	Redirect					redirect;
};

struct ServerConfig
{
	std::string					host;			// comme "127.0.0.1"
	std::string					root;
	std::vector<std::string>	index_files;
	std::set<std::string>		methods;
	Redirect					redirect;
	std::vector<LocationConfig>	locations;
};

struct Config
{
	std::vector<ServerConfig> servers;
};

#endif