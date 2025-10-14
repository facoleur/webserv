Ok je vais essayer de resumer ce que j'ai compris:
- `socket()` cree un socket, une sorte de file descriptor qui permet une communication dans deux sens (read, write)
- `bind()` relie le socket a une addresse concrete (un path) sur une machine (par exemple notre serveur)
- `listen()` marque un socket comme etant un socket passif, c'est-a-dire pret a accepter des demandes des connection
- `poll()` surveille un socket pour voir si il est pret a effectuer des operations d'input et/ou d'output, par exemple un read() ou un write(), 
sans se bloquer
- `accept()` accepte une demande de connection


> To accept connections, the following steps are performed:

           1.  A socket is created with socket(2).

           2.  The socket is bound to a local address using bind(2), so that other sockets may be connect(2)ed to it.

           3.  A willingness to accept incoming connections and a queue limit for incoming connections are specified with listen().

           4.  Connections are accepted with accept(2).
(source: `man listen`)


#### `socket()`
create an endpoint for communication<br>
`int socket(int domain, int type, int protocol);`
> socket() creates an endpoint for communication and returns a file descriptor that refers to that endpoint.
> The file descriptor returned by a successful call will be the lowest-numbered file descriptor not currently open for the process.
> The domain argument specifies a communication domain; this selects the protocol family which will be used for communication. These families are defined in <sys/socket.h>:

	Name         Purpose                                    Man page
 	AF_INET      IPv4 Internet protocols                    ip(7)
    AF_INET6     IPv6 Internet protocols                    ipv6(7)
    (...)

#### `bind()`
bind a name to a socket<br>
`int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);`
> When a socket is created with socket(2), it exists in a name space (address family) but has no address assigned to it. bind() assigns the address specified by addr to the socket referred to by the file descriptor sockfd. addrlen specifies the size, in bytes, of the address structure pointed to by addr. Traditionally, this operation is called “assigning a name to a socket”.
> It is normally necessary to assign a local address using bind() before a SOCK_STREAM socket may receive connections (see accept(2)).

#### `listen()`
listen for connections on a socket<br>
`int listen(int sockfd, int backlog);`
> listen() marks the socket referred to by sockfd as a passive socket, that is, as a socket that will be used to accept incoming connection requests using accept(2).
> The sockfd argument is a file descriptor that refers to a socket of type SOCK_STREAM or SOCK_SEQPACKET.
> The backlog argument defines the maximum length to which the queue of pending connections for sockfd may grow. If a connection request arrives when the queue is
full, the client may receive an error with an indication of ECONNREFUSED or, if the underlying protocol supports retransmission, the request may be ignored so that
a later reattempt at connection succeeds.

#### `accept()`
accept a connection on a socket<br>
`int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);`
> The accept() system call is used with connection-based socket types (SOCK_STREAM, SOCK_SEQPACKET). It extracts the first connection request on the queue of
pending connections for the listening socket, sockfd, creates a new connected socket, and returns a new file descriptor referring to that socket.  The  newly
created socket is not in the listening state.  The original socket sockfd is unaffected by this call.

> The  argument  sockfd  is  a socket that has been created with socket(2), bound to a local address with bind(2), and is listening for connections after a lis‐
ten(2).
(...)
In order to be notified of incoming connections on a socket, you can use select(2), poll(2), or epoll(7). A readable event will be delivered when a new connection is attempted and you may then call accept() to get a socket for that connection.

#### `poll()`
wait for some event on a file descriptor
`int poll(struct pollfd *fds, nfds_t nfds, int timeout);`
> poll() performs a similar task to select(2): it waits for one of a set of file descriptors to become ready to perform I/O.
> select() allows a program to monitor multiple file descriptors, waiting until one or more of the file descriptors become "ready" for some class of I/O operation
(e.g., input possible). A file descriptor is considered ready if it is possible to perform a corresponding I/O operation (e.g., read(2), or a sufficiently small write(2)) without blocking.
> The set of file descriptors to be monitored is specified in the fds argument, which is an array of structures of the following form:
```c
struct pollfd {
    int   fd;         /* file descriptor */
    short events;     /* requested events */
    short revents;    /* returned events */
};
```
> The caller should specify the number of items in the fds array in nfds.<br>
> The field `fd` contains a file descriptor for an open file. If this field is negative, then the corresponding events field is ignored and the revents field  returns zero. (This provides an easy way of ignoring a file descriptor for a single poll() call: simply negate the fd field. Note, however, that this technique can't be used to ignore file descriptor 0.)<br>
> The field `events` is an input parameter, a bit mask specifying the events the application is interested in for the file descriptor fd. This field may be specified
as zero, in which case the only events that can be returned in revents are POLLHUP, POLLERR, and POLLNVAL (see below).<br>
> The field `revents` is an output parameter, filled by the kernel with the events that actually occurred. The bits returned in revents can include any of those specified in events, or one of the values POLLERR, POLLHUP, or POLLNVAL. (These three bits are meaningless in the events field, and will be set in the revents  field whenever the corresponding condition is true.)<br>
> The timeout argument specifies the number of milliseconds that poll() should block waiting for a file descriptor to become ready. The call will block until either:
> - a file descriptor becomes ready;
> - the call is interrupted by a signal handler; or
> - the timeout expires.

> The bits that may be set/returned in events and revents are defined in <poll.h>:<br>
> - POLLIN There is data to read.
> - POLLHUP Hang up (only returned in revents; ignored in events).  Note that when reading from a channel such as a pipe or a stream socket, this event merely indicates that  the  peer closed its end of the channel.  Subsequent reads from the channel will return 0 (end of file) only after all outstanding data in the channel has been consumed.
> - POLLNVAL Invalid request: fd not open (only returned in revents; ignored in events).

#### `recv()`
receive a message from a socket
`ssize_t recv(int sockfd, void *buf, size_t len, int flags);`
> recv() is to receive messages from a socket.  It may be used to receive data on both connectionless and connection-oriented sockets.<br>
> The only difference between recv() and read(2) is the presence of flags.<br>
> returns the length of the message on successful completion.<br>
>

#### `connect()`
initiate a connection on a socket
`int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);`
(...)
> EINPROGRESS
> The socket is nonblocking and the connection cannot be completed immediately. (UNIX domain sockets failed with EAGAIN instead.) It is possible to select(2) or poll(2) for completion by selecting the socket for writing. After select(2) indicates writability, use getsockopt(2) to read the SO_ERROR option at level SOL_SOCKET to determine whether connect() completed successfully (SO_ERROR is zero) or unsuccessfully (SO_ERROR is one of the usual error codes listed here, explaining the reason for the failure).


### Claude on bind vs connect
#### Question
> My server uses bind() to listen, but I am also allowed to use connect() in the project, so what should that mean ? Is it to send the HTTP response back to the browser ?

#### Answer
Used 2 references
No, connect() is NOT for sending responses back
First principle: Once a connection exists, you use send()/recv() on both sides. You never call connect() or bind() again for that connection.

When accept() succeeds, you already have a bidirectional connection.

Here's what actually happens:

Browser (client) side:

socket() - create socket
connect(serverIP, serverPort) - initiate connection to your server
send() - send HTTP request
recv() - receive HTTP response
Your server side:

socket() - create listening socket
bind(serverIP, serverPort) - claim the port
listen() - mark socket as passive
accept() - returns a NEW socket for this specific client
recv() on new socket - read HTTP request
send() on new socket - send HTTP response back
close() the new socket
Key insight: accept() gives you a connected socket that's already linked to the client. You just send() to it. No connect() needed.

So when would YOUR server use connect()?
When your server acts as a CLIENT to someone else.

Possible scenarios in a web server project:

Proxy functionality: Your server forwards requests to another server

Client connects to you
You connect() to the backend server
You relay data between them
External API calls: Your server fetches data from another service

Handle client request
You connect() to api.example.com:80
Fetch data, send it back to client
Reverse proxy / load balancing: Distribute requests across multiple backend servers

Mental model:

Your server has two roles:

Server role: bind() + listen() + accept() → receive connections
Client role: socket() + connect() → make connections to others
For basic HTTP serving: You only use the server role. You never connect() anywhere - clients connect to YOU, and accept() gives you the sockets to talk back.

Summary:

You don't use connect() to respond to browsers - the socket from accept() is already connected in both directions. You'd only use connect() if your server needs to be a client to another server (proxy, API calls, etc.). For simple request → response, it's bind() → listen() → accept() → recv() → send() → close(). The accept() socket is the two-way channel for that conversation.