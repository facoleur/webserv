// feed.cpp
// pseudocode by lferro 24/10

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
