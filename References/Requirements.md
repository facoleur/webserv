# Webserv requirements
A list of requirements from the subject and the details from the references (HTTP specification, etc.)

## Template
##
### Requirement
> - ****<br>
### Implementation
### Responsibility ?


## Questions to clear
- "Your HTTP response status codes must be accurate"<br>
=> implement 3xx status codes for redirection ?
=> implement 401 Unauthorized ? ("*The request requires user authentication.*")
- Do we need to do anything to handle proxies ?

## General
### Requirements
> - **Although you are not required to implement the entire RFCs, reading it will help you develop the required features. The HTTP 1.0 is suggested as a reference point, but not enforced.**<br>
> - **We deliberately chose to offer only a subset of the HTTP RFC. In this context, the virtual host feature is considered out of scope. But you are allowed to implement it if you want.**<br>

### Implementation
- HTTP/1.0 ?

## Configuration file
### Requirements
> - **Your program must use a configuration file, provided as an argument on the command line, or available in a default path.**<br>
> - **You can take inspiration from the ’server’ section of the NGINX configuration file.**
> - **In the configuration file, you should be able to:**
	> 	- **Define all the interface:port pairs on which your server will listen to (defining multiple websites served by your program).**
	> 	- **Set up default error pages.**
	> 	- **Set the maximum allowed size for client request bodies.**
	> 	- **Specify rules or configurations on a URL/route (no regex required here), for a website, among the following:**
	> 		- **List of accepted HTTP methods for the route.**
	> 		- **HTTP redirection.**
	> 		- **Directory where the requested file should be located (e.g., if URL /kapouet is rooted to /tmp/www, URL /kapouet/pouic/toto/pouet will search for /tmp/www/pouic/toto/pouet).**
	> 		- **Enabling or disabling directory listing.**
	> 		- **Default file to serve when the requested resource is a directory.**
	> 		- **Uploading files from the clients to the server is authorized, and storage location is provided.**
> - **Execution of CGI, based on file extension (for example .php)**
> - **You must provide configuration files and default files to test and demonstrate that every feature works during the evaluation.**
> - **You can have other rules or configuration information in your file (e.g., a server name for a website if you plan to implement virtual hosts).**

### Implementation
- Configuration file parser

## CGI
### Requirements
> **Execution of CGI, based on file extension (for example .php). Here are some specific remarks regarding CGIs:**<br>
> - **Have a careful look at the environment variables involved in the web server-CGI communication. The full request and arguments provided by the client must be available to the CGI.**
> - **Just remember that, for chunked requests, your server needs to un-chunk them, the CGI will expect EOF as the end of the body.**
> - **The same applies to the output of the CGI. If no content_length is returned from the CGI, EOF will mark the end of the returned data.**
> - **The CGI should be run in the correct directory for relative path file access.**
> - **Your server should support at least one CGI (php-CGI, Python, and so forth).**

### Implementation
- ?

## Ports
### Requirement
> - **Your server must be able to listen to multiple ports to deliver different content (see Configuration file).**<br>
> - **In the configuration file, you should be able to:**<br>
**Define all the interface:port pairs on which your server will listen to (defining multiple websites served by your program)**

## Non-blocking server and client disconnections
### Requirement
> - **Your server must remain non-blocking at all times and properly handle client disconnections when necessary.**<br>

### Implementation
ChatGPT:
> a blocking web-server is similar to a phone call. you need to wait on-line to get a response and continue; where as a non-blocking web-server is like a sms service. you sms your request, do your things and react when you receive an sms back!
<br>

[StackOverflow](https://stackoverflow.com/questions/1926602/what-is-blocking-and-non-blocking-web-server-what-difference-between-both):
> Using a blocking socket, execution will wait (ie. "block") until the full socket operation has taken place. So, you can process any results/responses in your code immediately after. These are also called synchronous sockets. A non-blocking socket operation will allow execution to resume immediately and you can handle the server's response with a callback or event. These are called asynchronous sockets<br>

HTTP 1.0:
>  Except for experimental applications, current practice requires that the connection be established by the client prior to each request and closed by the server after sending the response. Both clients and servers should be aware that either party may close the connection prematurely, due to user action, automated time-out, or program failure, and should handle such closing in a predictable fashion. In any case, the closing of the connection by either or both parties always terminates the current request, regardless of its status.

## Requests shouldn't hang indefinitely
### Requirement
> - **A request to your server should never hang indefinitely.**<br>

### Implementation
- ?


## Parsing
### Requirements

### Implementation
> HTTP/1.0 defines the octet sequence CR LF as the end-of-line marker for all protocol elements except the Entity-Body (see Appendix B for tolerant applications). The end-of-line marker within an Entity-Body is defined by its associated media type, as described in Section 3.6.

       CRLF           = CR LF

> The following rules are used throughout this specification to describe basic parsing constructs. The US-ASCII coded character set is defined by [17].

       OCTET          = <any 8-bit sequence of data>
       CHAR           = <any US-ASCII character (octets 0 - 127)>
       UPALPHA        = <any US-ASCII uppercase letter "A".."Z">
       LOALPHA        = <any US-ASCII lowercase letter "a".."z">
       ALPHA          = UPALPHA | LOALPHA
       DIGIT          = <any US-ASCII digit "0".."9">
       CTL            = <any US-ASCII control character
                        (octets 0 - 31) and DEL (127)>
       CR             = <US-ASCII CR, carriage return (13)>
       LF             = <US-ASCII LF, linefeed (10)>
       SP             = <US-ASCII SP, space (32)>
       HT             = <US-ASCII HT, horizontal-tab (9)>
       <">            = <US-ASCII double-quote mark (34)>

## Execve
### Requirement
> - **You cannot execve another web server.**<br>

## Fork only for CGI
### Requirement
> - **You can’t use fork for anything other than CGI (like PHP, or Python, and so forth).**<br>

## Poll
### Allowed functions
#### Requirement
> - **Even though poll() is mentioned in the subject and evaluation sheet, you can use any equivalent function such as select(), kqueue(), or epoll()**.<br>

#### Implementation
- Use poll() ?

### Macro use
#### Requirement
> - **When using poll() or any equivalent call, you can use every associated macro or helper function (e.g., FD_SET for select()).**<br>

#### Implementation
- Re-read the code at the end to check this.

### Only 1 poll() for all I/O operations
#### Requirement
> - **It must be non-blocking and use only 1 poll() (or equivalent) for all the I/O operations between the clients and the server (listen included).**<br>

### Reading and writing
#### Requirements
> - **poll() (or equivalent) must monitor both reading and writing simultaneously.**<br>
> - **You must never do a read or a write operation without going through poll() (or equivalent).**<br>
> - **Checking the value of errno to adjust the server behaviour is strictly forbidden after performing a read or write operation.**<br>
> - **You are not required to use poll() (or equivalent) before read() to retrieve your configuration file.**<br>
> - **Because you have to use non-blocking file descriptors, it is possible to use read/recv or write/send functions with no poll() (or equivalent), and your server wouldn’t be blocking.**<br>
> - **But it would consume more system resources. Thus, if you attempt to read/recv or write/send on any file descriptor without using poll() (or equivalent), your grade will be 0.**.<br>

#### Implementation
- ?

## MacOS specifics
### Requirement
> - **Since macOS handles write() differently from other Unix-based OSes, you are allowed to use fcntl().**
> - **You must use file descriptors in non-blocking mode to achieve behaviour similar to that of other Unix OSes.**
> - **However, you are allowed to use fcntl() only with the following flags: F_SETFL, O_NONBLOCK and, FD_CLOEXEC. Any other flag is forbidden.**<br>

### Implementation
- ?

## Methods
### Requirement
> - **You need at least the GET, POST, and DELETE methods.**<br>

### Implementation
#### 8.1 GET
[Mozilla](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Methods/GET):<br>
> The GET HTTP method requests a representation of the specified resource. Requests using GET should only be used to request data and shouldn't contain a body.

**HTTP/1.0:**<br>
> The GET method means retrieve whatever information (in the form of an entity) is identified by the Request-URI. If the Request-URI refers to a data-producing process, it is the produced data which shall be returned as the entity in the response and not the source text of the process, unless that text happens to be the output of the process.

> The semantics of the GET method changes to a "conditional GET" if the request message includes an If-Modified-Since header field. A conditional GET method requests that the identified resource be transferred only if it has been modified since the date given by the If-Modified-Since header, as described in Section 10.9. The conditional GET method is intended to reduce network usage by allowing cached entities to be refreshed without requiring multiple requests or transferring unnecessary data. 

#### D.1.2 DELETE
[Mozilla](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Methods/DELETE):<br>
> The DELETE HTTP method asks the server to delete a specified resource.
> The DELETE method has no defined semantics for the message body, so this should be empty.

**HTTP/1.0:**<br>
> The DELETE method requests that the origin server delete the resource identified by the Request-URI.

#### 8.3 POST
[Mozilla](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Methods/GET):<br>
> The POST HTTP method sends data to the server. The type of the body of the request is indicated by the Content-Type header.

> The difference between PUT and POST is that PUT is idempotent: calling it once is no different from calling it several times successively (there are no side effects). Successive identical POST requests may have additional effects, such as creating the same order several times.

> HTML forms typically send data using POST and this usually results in a change on the server. For HTML forms the format/encoding of the body content is determined by the enctype attribute of the \<form\> element or the formenctype attribute of the \<input\> or \<button\> elements. The encoding may be one of the following:

- application/x-www-form-urlencoded: the keys and values are encoded in key-value tuples separated by an ampersand (&), with an equals symbol (=) between the key and the value (e.g., first-name=Frida&last-name=Kahlo). Non-alphanumeric characters in both keys and values are percent-encoded: this is the reason why this type is not suitable to use with binary data and you should use multipart/form-data for this purpose instead.
- multipart/form-data: each value is sent as a block of data ("body part"), with a user agent-defined delimiter (for example, boundary="delimiter12345") separating each part. The keys are described in the Content-Disposition header of each part or block of data.
- text/plain

> When the POST request is sent following a fetch() call, or for any other reason than an HTML form, the body can be any type. As described in the HTTP 1.1 specification, POST is designed to allow a uniform method to cover the following functions:

- Annotation of existing resources
- Posting a message to a bulletin board, newsgroup, mailing list, or similar group of articles
- Adding a new user through a signup form
- Providing a block of data, such as the result of submitting a form, to a data-handling process
- Extending a database through an append operation

<br>

**HTTP/1.0:**
<br>
> The POST method is used to request that the destination server accept the entity enclosed in the request as a new subordinate of the resource identified by the Request-URI in the Request-Line. POST is designed to allow a uniform method to cover the following functions:

> - Annotation of existing resources;
> - Posting a message to a bulletin board, newsgroup, mailing list, or similar group of articles;
> - Providing a block of data, such as the result of submitting a form [3], to a data-handling process;
> - Extending a database through an append operation. 

> The actual function performed by the POST method is determined by the server and is usually dependent on the Request-URI. The posted entity is subordinate to that URI in the same way that a file is subordinate to a directory containing it, a news article is subordinate to a newsgroup to which it is posted, or a record is subordinate to a database.<br>

> A successful POST does not require that the entity be created as a resource on the origin server or made accessible for future reference. That is, the action performed by the POST method might not result in a resource that can be identified by a URI. In this case, either 200 (ok) or 204 (no content) is the appropriate response status, depending on whether or not the response includes an entity that describes the result.<br>

> If a resource has been created on the origin server, the response should be 201 (created) and contain an entity (preferably of type "text/html") which describes the status of the request and refers to the new resource.<br>

> A valid Content-Length is required on all HTTP/1.0 POST requests. An HTTP/1.0 server should respond with a 400 (bad request) message if it cannot determine the length of the request message's content.<br>

> Applications must not cache responses to a POST request because the application has no way of knowing that the server would return an equivalent response on some future request. 

## File upload
### Requirement
> - **Clients must be able to upload files.**<br>
### Implementation
- POST ?

## NGINX use
### Requirement
> - **NGINX may be used to compare headers and answer behaviours (pay attention to differences between HTTP versions).**<br>

## HTTP response status codes
### Requirement
> - **Your HTTP response status codes must be accurate**<br>

### Implementation
#### 1xx codes (informational)
HTML 1.0:
> HTTP/1.0 does not define any 1xx status codes and they are not a valid response to a HTTP/1.0 request.

#### 2xx codes (successful)
##### 200 OK (GET, POST)
The request has succeeded. The information returned with the response is dependent on the method used in the request, as follows:

- GET: an entity corresponding to the requested resource is sent in the response;
(...)<br>
- POST: an entity describing or containing the result of the action.

##### 201 Created (POST)
HTML 1.0:
> The request has been fulfilled and resulted in a new resource being created. The newly created resource can be referenced by the URI(s) returned in the entity of the response. The origin server should create the resource before using this Status-Code. If the action cannot be carried out immediately, the server must include in the response body a description of when the resource will be available; otherwise, the server should respond with 202 (accepted).
> Of the methods defined by this specification, only POST can create a resource.

##### 202 Accepted
> The request has been accepted for processing, but the processing has not been completed. The request may or may not eventually be acted upon, as it may be disallowed when processing actually takes place. There is no facility for re-sending a status code from an asynchronous operation such as this.

> The 202 response is intentionally non-committal. Its purpose is to allow a server to accept a request for some other process (perhaps a batch-oriented process that is only run once per day) without requiring that the user agent's connection to the server persist until the process is completed. The entity returned with this response should include an indication of the request's current status and either a pointer to a status monitor or some estimate of when the user can expect the request to be fulfilled.

##### 204 No Content
> The server has fulfilled the request but there is no new information to send back. If the client is a user agent, it should not change its document view from that which caused the request to be generated. This response is primarily intended to allow input for scripts or other actions to take place without causing a change to the user agent's active document view. The response may include new metainformation in the form of entity headers, which should apply to the document currently in the user agent's active view.

#### Redirection 3xx
> This class of status code indicates that further action needs to be taken by the user agent in order to fulfill the request. The action required may be carried out by the user agent without interaction with the user if and only if the method used in the subsequent request is GET or HEAD. A user agent should never automatically redirect a request more than 5 times, since such redirections usually indicate an infinite loop.

##### 300 Multiple Choices
> This response code is not directly used by HTTP/1.0 applications, but serves as the default for interpreting the 3xx class of responses.<br>
> The requested resource is available at one or more locations. Unless it was a HEAD request, the response should include an entity containing a list of resource characteristics and locations from which the user or user agent can choose the one most appropriate. If the server has a preferred choice, it should include the URL in a Location field; user agents may use this field value for automatic redirection.

###### 301 Moved Permanently
###### 302 Moved Temporarily
###### 304 Not Modified (conditional GET)
> If the client has performed a conditional GET request and access is allowed, but the document has not been modified since the date and time specified in the If-Modified-Since field, the server must respond with this status code and not send an Entity-Body to the client. Header fields contained in the response should only include information which is relevant to cache managers or which may have changed independently of the entity's Last-Modified date. Examples of relevant header fields include: Date, Server, and Expires. A cache should update its cached entity to reflect any new field values given in the 304 response.

#### Client Error 4xx
> The 4xx class of status code is intended for cases in which the client seems to have erred. If the client has not completed the request when a 4xx code is received, it should immediately cease sending data to the server. Except when responding to a HEAD request, the server should include an entity containing an explanation of the error situation, and whether it is a temporary or permanent condition. These status codes are applicable to any request method.

	>   Note: If the client is sending data, server implementations on TCP should be careful to ensure that the client acknowledges receipt of the packet(s) containing the response prior to closing the input connection. If the client continues sending data to the server after the close, the server's controller will send a reset packet to the client, which may erase the client's unacknowledged input buffers before they can be read and interpreted by the HTTP application. 

##### 400 Bad Request
> The request could not be understood by the server due to malformed syntax. The client should not repeat the request without modifications.

##### 401 Unauthorized
> The request requires user authentication. The response must include a WWW-Authenticate header field (Section 10.16) containing a challenge applicable to the requested resource. The client may repeat the request with a suitable Authorization header field (Section 10.2). If the request already included Authorization credentials, then the 401 response indicates that authorization has been refused for those credentials. If the 401 response contains the same challenge as the prior response, and the user agent has already attempted authentication at least once, then the user should be presented the entity that was given in the response, since that entity may include relevant diagnostic information. HTTP access authentication is explained in Section 11.

##### 403 Forbidden
> The server understood the request, but is refusing to fulfill it. Authorization will not help and the request should not be repeated. If the request method was not HEAD and the server wishes to make public why the request has not been fulfilled, it should describe the reason for the refusal in the entity body. This status code is commonly used when the server does not wish to reveal exactly why the request has been refused, or when no other response is applicable.

##### 404 Not Found
> The server has not found anything matching the Request-URI. No indication is given of whether the condition is temporary or permanent. If the server does not wish to make this information available to the client, the status code 403 (forbidden) can be used instead.

#### Server Error 5xx
> Response status codes beginning with the digit "5" indicate cases in which the server is aware that it has erred or is incapable of performing the request. If the client has not completed the request when a 5xx code is received, it should immediately cease sending data to the server. Except when responding to a HEAD request, the server should include an entity containing an explanation of the error situation, and whether it is a temporary or permanent condition. These response codes are applicable to any request method and there are no required header fields.

##### 500 Internal Server Error
> The server encountered an unexpected condition which prevented it from fulfilling the request.

##### 501 Not Implemented
> The server does not support the functionality required to fulfill the request. This is the appropriate response when the server does not recognize the request method and is not capable of supporting it for any resource.

##### 502 Bad Gateway
> The server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request.

##### 503 Service Unavailable
The server is currently unable to handle the request due to a temporary overloading or maintenance of the server. The implication is that this is a temporary condition which will be alleviated after some delay.

>    Note: The existence of the 503 status code does not imply that a server must use it when becoming overloaded. Some servers may wish to simply refuse the connection. 

## Default error pages
### Requirement
> - **Your server must have default error pages if none are provided.**<br>

### Implementation
- Test if error pages are all provided.

## Static website
### Requirement
> - ****<br>
You must be able to serve a fully static website.

### Implementation
- Make a static website and test it

## Server availability
### Requirement
> - **Stress test your server to ensure it remains available at all times.**<br>

### Implementation
- Testers ?
- Make our own testers ?

## Browser compatibility
### Requirement
> - **Your server must be compatible with standard web browsers of your choice**<br>

### Implementation
- Perform the various tests (error pages, static website, stress tests, etc.) with various browsers.

## Testing
### Requirements
> - **If you have a question about a specific behaviour, you can compare your program’s behaviour with NGINX’s.**<br>
> - **We have provided a small tester. Using it is not mandatory if everything works fine with your browser and tests, but it can help you find and fix bugs.**<br>
> - **Resilience is key. Your server must remain operational at all times.**<br>
> - **Do not test with only one program. Write your tests in a more suitable language, such as Python or Golang, among others, even in C or C++ if you prefer.**<br>

### Implementation
- to be discussed later on ?

