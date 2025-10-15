# Notes on Webserv project

## Requirements
### Non-blocking server
> a blocking web-server is similar to a phone call. you need to wait on-line to get a response and continue; where as a non-blocking web-server is like a sms service. you sms your request,do your things and react when you receive an sms back!
<br><br>
> Using a blocking socket, execution will wait (ie. "block") until the full socket operation has taken place. So, you can process any results/responses in your code immediately after. These are also called synchronous sockets. A non-blocking socket operation will allow execution to resume immediately and you can handle the server's response with a callback or event. These are called asynchronous sockets
<br>([StackOverflow](https://stackoverflow.com/questions/1926602/what-is-blocking-and-non-blocking-web-server-what-difference-between-both))

### Select() and poll()
> select()  allows a program to monitor multiple file descriptors, waiting until one or more of the file descriptors become "ready" for some class of I/O operation (e.g., input possible).  A file descriptor is considered ready if it is
       possible to perform a corresponding I/O operation (e.g., read(2), or a sufficiently small write(2)) without blocking.<br>
>    select() can monitor only file descriptors numbers that are less than FD_SETSIZE; poll(2) and epoll(7) do not have this limitation.<br>

> poll, ppoll - wait for some event on a file descriptor

SYNOPSIS
       #include <poll.h>

       int poll(struct pollfd *fds, nfds_t nfds, int timeout);
(...)
> The timeout argument specifies the number of milliseconds that poll() should block waiting for a file descriptor to become ready.  The call will block until either:

       • a file descriptor becomes ready;

       • the call is interrupted by a signal handler; or

       • the timeout expires.
> Specifying a timeout of zero causes poll() to return immediately, even if no file descriptors are ready.


## Notions
### [Socket](https://www.ibm.com/docs/en/zos/2.4.0?topic=services-what-is-socket)
> A socket can be thought of as an endpoint in a two-way communication channel. Socket routines create the communication channel, and the channel is used to send data between application programs either locally or over networks. Each socket within the network has a unique name associated with it called a socket descriptor—a fullword integer that designates a socket and allows application programs to refer to it when needed.<br>
> Using an electrical analogy, you can think of the communication channel as the electrical wire with its plug and think of the port, or socket, as the electrical socket or outlet, as shown in Figure 1.<br>
> As with file access, user processes ask the operating system to create a socket when one is needed. The system returns an integer, the socket descriptor (sd), that the application uses every time it wants to refer to that socket. The main difference between sockets and files is that the operating system binds file descriptors to a file or device when the open() call creates the file descriptor. With sockets, application programs can choose to either specify the destination each time they use the socket—for example, when sending datagrams—or to bind the destination address to the socket.<br>
>Sockets behave in some respects like UNIX files or devices, so they can be used with such traditional operations as read() or write(). For example, after two application programs create sockets and open a connection between them, one program can use write() to send a stream of data, and the other can use read() to receive it. Because each file or socket has a unique descriptor, the system knows exactly where to send and to receive the data. <br>
