#include <iostream>
#include <vector>

std::vector<std::string> &split_line(std::vector<std::string> &split, std::string &line);

int main(void)
{
	std::vector<std::string>    split;
	std::string					_requestLine[10];

	_requestLine[0] = "";
	_requestLine[1] = "GET/HTTP/1.1";
	_requestLine[2] = "GET/ HTTP/1.1";
	_requestLine[3] = "GET / HTTP/1.1";
	_requestLine[4] = "GET / HTTP/1.1 ";
	_requestLine[5] = " GET / HTTP/1.1";
	_requestLine[6] = "       ";

    for (size_t k = 0; k < 7; k++)
	{
		std::cout << "requestLine[" << k << "]: '" << _requestLine[k] << "'" << std::endl;
		split = split_line(split, _requestLine[k]);
		if (!split.empty())
			std::cout << "=> split: (size: " << split.size() << ") - {"<< split[0] << "}-{" << split[1] << "}-{" << split[2] << "}" << std::endl;
	}
}

std::vector<std::string> &split_line(std::vector<std::string> &split, std::string &line)
{
    size_t                      pos;

	for (size_t i = 0; i < 2; i++)
	{
		pos = line.find(' ');
		if (pos == line.npos)
		{
			// std::cout << "requestLine[" << k << "]: couldn't find space" << std::endl << std::endl;
			split.clear();
			return split;
		}
		// else
		// 	std::cout << "found space" << std::endl;
		// std::cout << "pushing back: " << line.substr(0, pos) << std::endl;
		split.push_back(line.substr(0, pos));
		line = line.substr(pos + 1);
	}
	// std::cout << "pushing back: " << line.substr(0) << std::endl;
	split.push_back(line.substr(0));
	return split;
}

