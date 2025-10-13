## Webserv first roadmap

## Config
- [ ] Write the basic config
- [ ] Parse and define the structure the config into a class 

## Core
- [ ] Create the main class
- [ ] Create the sockets
- [ ] Bind sockets to ports
- [ ] Make sockets non-blocking
- [ ] create the main loop with `epoll()`
- [ ] handle file uploads

## Req & res
- [ ] Define the request structure
- [ ] Parse http requests
- [ ] Define the response structure
- [ ] Handle all required status codes
- [ ] `>>` overload for logging req and res

## Misc
- [ ] Makefile 
- [ ] Write or use existing CGI
- [ ] Create html pages (errors, indexes, etc)

## Tester
- [ ] Write tests for parsing components
- [ ] Write tests for the response generation
- [ ] Write tests for the server behavior

--- 

## Help
- We can run Nginx locally to compare behavior
- Log requests and responses 

###  Handle big files:
#### Send
  - define CHUNK_SIZE
	- read file in chunks of CHUNK_SIZE
	- send chunks of CHUNK_SIZE
	- define and handle timeouts
#### Receive
	- read header and look for Content-Length
	- open destination file
