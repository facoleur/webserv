#ifndef	PARSER_HPP
#define	PARSER_HPP

#include "Config.hpp"
#include <stdexcept>
#include <string>
#include <vector>

class ParseError : public std::runtime_error
{
	public:
		ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

class ConfigParser
{
	public:
		ConfigParser();
		Config parseFile(const std::string& path);
		Config parseString(const std::string& text);

	private:
		struct Token
		{
			std::string	s;
			size_t		line;
			size_t		col;
		};
		std::vector<Token> tokenize(const std::string& text);

		void parseConfig();
		void parseServer();
		void parseLocation(ServerConfig& srv);

		bool accept(const std::string& kw);
		void expect(const std::string& kw, const char* err);
		Token next();				// consomme
		const Token& peek() const;	// sans consommer
		bool eof() const;

		static bool isMethod(const std::string& m);
		static int  toInt(const std::string& s);

		Config cfg_;
		std::vector<Token> toks_;
		size_t i_;
};

#endif