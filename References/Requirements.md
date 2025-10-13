# Webserv requirements
A list of requirements from the subject and the details from the references (HTTP specification, etc.)

## Template
##
### Requirement
> - ****<br>
### Implementation
### Responsibility ?

## Questions
### To clear
- "Your HTTP response status codes must be accurate"<br>
=> implement 401 Unauthorized ? ("*The request requires user authentication.*")

## Cleared
- HTTP/1.1 or HTTP/1.0 ? Should we handle or reject (or ?) HTTP/0.9 and HTTP/2+ requests ?
=> how to handle different versions in requests ?
=> tfrily: you can say no to anything not 1.1
- Do we need to do anything to handle proxies ? => tfrily, kly: no
- cache ? => tfrily, kly: no
- Requests:
	- conditional also (e.g. GET) ? => tfrily: no
	- body on DELETE: reject request or ignore ? => tfrily: read the spec and you decide, justify your choice(s)
- How to implement "non-blocking" ? Is it with poll() or something more general about how the program works ? => tfrily: [many things]
- Redirections: what are they and whether to "implement 3xx status codes for redirection" ? => tfrily: yes + add header "location" in response. What they are: see config (something has moved ?)

- What are redirections and how to handle them ?

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
- See error-codes.md

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
- Use LLMs to generate tests

