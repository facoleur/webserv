// RequestParser.cpp

#include "../inc/RequestParser.hpp"

RequestParser::RequestParser(void) : _parserState(REQ_PARSE_START), _parsingPhase(PARSING_REQUEST_LINE), 
_statusCode(NO_STATUS), _contentLength(0), _accumulator(), _firstSection(), _requestLine(), _headers(), _body() {
}

void RequestParser::parseRequestLine(bool pureRequestLine)
{
    /* split line */
    /* checks:
        - only three elements 
        - not more than two spaces
    */
}

void RequestParser::handleParseError(Request &req, std::queue<Request> &reqQueue)
{
	req.setValidity(INVALID_REQUEST);
	reqQueue.add(req);
	_parserState = REQ_PARSE_ERROR;
}

void RequestParser::feed(char* buf, std::queue<Request> &reqQueue) 
{
    size_t      pos;
    Request     req;

    _accumulator += buf;

	while (!_accumulator.empty())
	{
		switch (_parsingPhase)
		{
			case PARSING_REQUEST_LINE:
			case PARSING_HEADERS:
				pos = _accumulator.find(CRLF + CRLF);
				if (pos == _accumulator.npos && _accumulator.size() >= READ_BUF_SIZE)
					return handleParseError(req, reqQueue);
				else if (pos == _accumulator.npos)
				{
					_firstSection = _accumulator;
					_accumulator.clear();
					_parserState = REQ_PARSE_PARTIAL;
					return;
				}
				_firstSection += _accumulator.substr(0, pos);
				if (_firstSection.size() > READ_BUF_SIZE)
					return handleParseError(req, reqQueue);
				_accumulator = _accumulator.substr(pos);
				break;
			case PARSING_BODY:
				// ...
				break;
			case PARSING_COMPLETE:
				// ??
				break;
			default:
				break;
		}

		/* Parse the line */
        try
        {
            switch (_parsingPhase) // internal
            {
                case PARSING_REQUEST_LINE:
					pos = _firstSection.find(CRLF);
					if (pos == _firstSection.npos)
					{
						parseRequestLine(true);
						_parsingPhase = PARSING_COMPLETE;
					}
					else
					{
						parseRequestLine(false);
						_parsingPhase = PARSING_HEADERS;
					}
                    break;
                // case PARSING_HEADERS:
                //     parseHeaders();
				//	   if (_parsingPhase)
                //     _parsingPhase = PARSING_HEADERS;
                //     break;
                // case PARSING_BODY:
                //     ....
                //     break;
                case PARSING_COMPLETE:
					req.validateRequest();
					req_queue.push(req);
					if (req.getValidity() == INVALID_REQUEST)
						break;
                    continue;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
            _parserState = REQ_PARSE_ERROR;
            break;
        }
    }
	_parserState = REQ_PARSE_COMPLETE;
}

buffer
isHeaderDone
isChunked
contentLength
headersString
bodyString


ParserState feed(string data, queue<Request> &req) {
	buffer.append(data);

	while (true) {

		// handle headers
		if (!isheaderDone) {

			headerEnd = buffer.find(/r/n/r/n)

			if (buffer.size <= 4 * READ_SIZE)
				return REQ_ERROR

			if (headerEnd == npos) // not found
				setState REQ_PARTIAL
				return

			headersString = buffer.substr(headerEnd);
			Request req;
			parseHeaders(headersString, &req);

			isHeaderDone = true;

			isChunked = isChunked(req.headers);

			buffer.erase(headersString)

			if (!isChunked) contentlength = getcontentlength(req.headers)

			if (!isChunked && !contentLength) {
				requests.push(req);
				clearParser();
			}

			continue;
		}

		// handle body
		if (chunked) {
			parseChunkedBody(buffer);
		}
		if (contentlength) {
			if (buffer.size() < contentlength)
				return PARTIAL
		}
		req.body = buffer.substr(contentlength)
		buffer.erase(body)
		requests.push(req)
		clearParser()

		return ERROR;

	}
}



// HOW TO USE IT IN SERVER LOOP
// Read 1000 bytes:

// request:
// GET /trucvalide HTTP/1.1

// response:
//

// GET/ HTTP/1.0 => poubelle
// Host: 127.0.0.1 => poubelle
// GET /root HTTP/1.1
// Host: blabla

/* IN SERVER.CPP LOOP:

    while (1)
    {
        std::map<int, RequestParser> request_parsers;
        Request *req = new Request();

        // ...

        poll(pfds);
        ssize_t ret = read(sockFd, buf, READ_BUF_SIZE);
        if (read <= 0)
            return handle_read_error...
		request_parsers[sockFd].feed(buf, req); // call the parser on the buffer to fill the Request
		enum ParserState ps = request_parsers[sockFd].getState());              
		if (ps == REQ_PARSE_PARTIAL)
			continue;
		handle_requests(req_queue);
		if (ps == REQ_PARSE_ERROR)
			close(sockFd);
	}

handle_requests(queue requests)
{
        if (fd == POLLOUT)
                read(fichier, buf, 8196);
                write(clientFd, buf, 8196);
}
*/