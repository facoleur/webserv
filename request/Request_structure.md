# Structure of an HTTP request

## To clarify
- CRLF: see "Components" => only CRLF or alternatives too ? => tfrily: see RFC 9111 () and 9110 (HTTP semantics)
- Source of truth for body in GET: Content-Length ?

## HTTP request structure
Following [A typical HTTP session - Mozilla](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Session) and [HTTP headers](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers) and [HTTP/1.1](https://www.w3.org/Protocols/rfc2616/rfc2616-sec9.html#sec9.7)

### Components
Text directives separated by CR LF (carriage return – ASCII 13 + line feed – ASCII 10 (nl on OS X))

HTTP/1.0:
> Media subtypes of the "text" type use CRLF as the text line break when in canonical form. However, HTTP allows the transport of text media with plain CR or LF alone representing a line break when used consistently within the Entity-Body. HTTP applications must accept CRLF, bare CR, and bare LF as being representative of a line break in text media received via HTTP.

**=> to clarify !**

### Block 1: First line
- Request method (GET, POST, DELETE or UNKNOWN in our case)
- path of the document, as an absolute URL without the protocol or domain name
- HTTP protocol version

### Block 2: Headers (multiple lines)
- information on what type of data is appropriate (e.g. language, MIME types, etc.), or other data altering its behavior (e.g. not sending an answer if it is already cached)
- ends with an empty line
- in HTTP/1.X, a header is a case-insensitive name followed by a colon, then optional whitespace which will be ignored, and finally by its values (e.g. `Allow: POST`)
- in HTTP/2 and above, headers are displayed in lowercase when viewed in developer tools (..)

### Block 3: Data block (optional)
- May contain further data; mainly used by POST
- If used by GET or DELETE => error

## General structure
The class `Request` has:
- a method : GET, POST, DELETE, UNKNOWN
- a path : /index.html (absolute URL without the protocol or domain name)
- a query string: ?q=webserv&lang=en (optional, parsed by the CGI)
- an HTTP protocol version: HTTP/1.1
- a set of headers, which can have duplicates (n.b.: careful with case-insensitivity)
- a body (only for POST) (must <= MAX_BODY_SIZE, see Config file)
- a validity status

note: no explicit information about CGI, which is handled afterwards at the application level (the Request is a protocol-level data structure)

## Implementation
See Request.hpp and RequestParser.hpp

How the main loop will handle this:
```cpp
RequestParser req_parser;
Request	*req = req_parser.parse(socketFd);
if (req == NULL)
	handle_parse_error(socketFd, 400);
else if (req->validate() == INVALID_REQUEST)
	handle_request_error(socketFd, req);
else
	handle_request(socketFd, req);

delete req;
```

n.b. (Claude):
> Why socketFd as parameter in handlers?
Because each handler needs to send an HTTP response back to the client.
> When you call handle_parse_error(socketFd, 400), that function must:
> - Format an HTTP 400 response
> - Call send(socketFd, ...) to transmit it back to the client
> The socketFd is your communication channel with the client. Without it, you can't reply.
> First principle: The socket is a bidirectional pipe. You read the request from it (in parser), then write the response to it (in handlers). Same file descriptor, opposite direction.

### Methods
- GET:
	- no body. If body OR header Content-Length, undefined => reject the request with a 4xx client error response ([Mozilla - GET](https://developer.mozilla.org/en-US/docs/Glossary/Cacheable)). => we choose: 400: Bad Request

- DELETE:
	- Body ? => Ignore or error

[HTTP/1.1](https://www.w3.org/Protocols/rfc2616/rfc2616-sec9.html#sec9.7):
> The DELETE method requests that the origin server delete the resource identified by the Request-URI. This method MAY be overridden by human intervention (or other means) on the origin server. The client cannot be guaranteed that the operation has been carried out, even if the status code returned from the origin server indicates that the action has been completed successfully. However, the server SHOULD NOT indicate success unless, at the time the response is given, it intends to delete the resource or move it to an inaccessible location.

> A successful response SHOULD be 200 (OK) if the response includes an entity describing the status, 202 (Accepted) if the action has not yet been enacted, or 204 (No Content) if the action has been enacted but the response does not include an entity.

### Headers
#### Required headers:
- `Content-length` for POST requests. cf. HTTP/1.0:
> A valid Content-Length is required on all HTTP/1.0 POST requests. An HTTP/1.0 server should respond with a 400 (bad request) message if it cannot determine the length of the request message's content.<br>
- `Content-Type` for POST requests. cf. HTTP/1.0:
> The POST HTTP method sends data to the server. The type of the body of the request is indicated by the  header.
- `Host`. cf. Claude:
> Even though HTTP/1.0 doesn't strictly require it, browsers send it and you'll need it for routing

#### Implementation
- store everything as a string, then validate each header
- duplicate headers: possible, but only for if the entire field-value for that header field is defined as a comma-separated list. See
[HTTP RFC2616](https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2):
	> Multiple message-header fields with the same field-name MAY be present in a message if and only if the entire field-value for that header field is defined as a comma-separated list [i.e., #(values)]. It MUST be possible to combine the multiple header fields into one "field-name: field-value" pair, without changing the semantics of the message, by appending each subsequent field-value to the first, each separated by a comma. The order in which header fields with the same field-name are received is therefore significant to the interpretation of the combined field value, and thus a proxy MUST NOT change the order of these field values when a message is forwarded
