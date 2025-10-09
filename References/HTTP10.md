Link: https://www.w3.org/Protocols/HTTP/1.0/spec.html

## 5. REQUEST
A request message from a client to a server includes, within the first line of that message, the method to be applied to the resource, the identifier of the resource, and the protocol version in use. For backwards compatibility with the more limited HTTP/0.9 protocol, there are two valid formats for an HTTP request:

       Request        = Simple-Request | Full-Request

       Simple-Request = "GET" SP Request-URI CRLF

       Full-Request   = Request-Line              ; Section 5.1
                        *( General-Header         ; Section 4.3
                         | Request-Header         ; Section 5.2
                         | Entity-Header )        ; Section 7.1
                        CRLF
                        [ Entity-Body ]           ; Section 7.2

If an HTTP/1.0 server receives a Simple-Request, it must respond with an HTTP/0.9 Simple-Response. An HTTP/1.0 client capable of receiving a Full-Response should never generate a Simple-Request.

### 5.1 Request-Line
The Request-Line begins with a method token, followed by the Request-URI and the protocol version, and ending with CRLF. The elements are separated by SP characters. No CR or LF are allowed except in the final CRLF sequence.

       Request-Line   = Method SP Request-URI SP HTTP-Version CRLF

Note that the difference between a Simple-Request and the Request-Line of a Full-Request is the presence of the HTTP-Version field and the availability of methods other than GET.

#### 5.1.1 Method
The Method token indicates the method to be performed on the resource identified by the Request-URI. The method is case-sensitive.

       Method         = "GET"                    ; Section 8.1
                      | "HEAD"                   ; Section 8.2
                      | "POST"                   ; Section 8.3
                      | extension-method

       extension-method = token

The list of methods acceptable by a specific resource can change dynamically; the client is notified through the return code of the response if a method is not allowed on a resource. Servers should return the status code 501 (not implemented) if the method is unrecognized or not implemented.

The methods commonly used by HTTP/1.0 applications are fully defined in Section 8.

#### 5.1.2 Request-URI
The Request-URI is a Uniform Resource Identifier (Section 3.2) and identifies the resource upon wh ich to apply the request.

       Request-URI    = absoluteURI | abs_path

The two options for Request-URI are dependent on the nature of the request.

The absoluteURI form is only allowed when the request is being made to a proxy. The proxy is requested to forward the request and return the response. If the request is GET or HEAD and a prior response is cached, the proxy may use the cached message if it passes any restrictions in the Expires header field. Note that the proxy may forward the request on to another proxy or directly to the server specified by the absoluteURI. In order to avoid request loops, a proxy must be able to recognize all of its server names, including any aliases, local variations, and the numeric IP address. An example Request-Line would be:

       GET /TheProject.html HTTP/1.0

The most common form of Request-URI is that used to identify a resource on an origin server or gateway. In this case, only the absolute path of the URI is transmitted (see Section 3.2.1, abs_path). For example, a client wishing to retrieve the resource above directly from the origin server would create a TCP connection to port 80 of the host "www.w3.org" and send the line:

       GET /pub/WWW/TheProject.html HTTP/1.0

followed by the remainder of the Full-Request. Note that the absolute path cannot be empty; if none is present in the original URI, it must be given as "/" (the server root).

The Request-URI is transmitted as an encoded string, where some characters may be escaped using the "% HEX HEX" encoding defined by RFC 1738 [4]. The origin server must decode the Request-URI in order to properly interpret the request.

### 5.2 Request Header Fields
The request header fields allow the client to pass additional information about the request, and about the client itself, to the server. These fields act as request modifiers, with semantics equivalent to the parameters on a programming language method (procedure) invocation.

       Request-Header = Authorization            ; Section 10.2
                      | From                     ; Section 10.8
                      | If-Modified-Since        ; Section 10.9
                      | Referer                  ; Section 10.13
                      | User-Agent               ; Section 10.15

Request-Header field names can be extended reliably only in combination with a change in the protocol version. However, new or experimental header fields may be given the semantics of request header fields if all parties in the communication recognize them to be request header fields. Unrecognized header fields are treated as Entity-Header fields.

## 6. RESPONSE
After receiving and interpreting a request message, a server responds in the form of an HTTP response message.

       Response        = Simple-Response | Full-Response

       Simple-Response = [ Entity-Body ]

       Full-Response   = Status-Line              ; Section 6.1
                         *( General-Header        ; Section 4.3
                          | Response-Header       ; Section 6.2
                          | Entity-Header )       ; Section 7.1
                         CRLF
                         [ Entity-Body ]          ; Section 7.2

A Simple-Response should only be sent in response to an HTTP/0.9 Simple-Request or if the server only supports the more limited HTTP/0.9 protocol. If a client sends an HTTP/1.0 Full-Request and receives a response that does not begin with a Status-Line, it should assume that the response is a Simple-Response and parse it accordingly. Note that the Simple-Response consists only of the entity body and is terminated by the server closing the connection.

### 6.1 Status-Line
The first line of a Full-Response message is the Status-Line, consisting of the protocol version followed by a numeric status code and its associated textual phrase, with each element separated by SP characters. No CR or LF is allowed except in the final CRLF sequence.

       Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF

Since a status line always begins with the protocol version and status code

       "HTTP/" 1*DIGIT "." 1*DIGIT SP 3DIGIT SP

(e.g., "HTTP/1.0 200 "), the presence of that expression is sufficient to differentiate a Full-Response from a Simple-Response. Although the Simple-Response format may allow such an expression to occur at the beginning of an entity body, and thus cause a misinterpretation of the message if it was given in response to a Full-Request, most HTTP/0.9 servers are limited to responses of type "text/html" and therefore would never generate such a response.

#### 6.1.1 Status Code and Reason Phrase
The Status-Code element is a 3-digit integer result code of the attempt to understand and satisfy the request. The Reason-Phrase is intended to give a short textual description of the Status-Code. The Status-Code is intended for use by automata and the Reason-Phrase is intended for the human user. The client is not required to examine or display the Reason-Phrase.

The first digit of the Status-Code defines the class of response. The last two digits do not have any categorization role. There are 5 values for the first digit:

    1xx: Informational - Not used, but reserved for future use
    2xx: Success - The action was successfully received, understood, and accepted.
    3xx: Redirection - Further action must be taken in order to complete the request
    4xx: Client Error - The request contains bad syntax or cannot be fulfilled
    5xx: Server Error - The server failed to fulfill an apparently valid request 

The individual values of the numeric status codes defined for HTTP/1.0, and an example set of corresponding Reason-Phrase's, are presented below. The reason phrases listed here are only recommended -- they may be replaced by local equivalents without affecting the protocol. These codes are fully defined in Section 9.

       Status-Code    = "200"   ; OK
                      | "201"   ; Created
                      | "202"   ; Accepted
                      | "204"   ; No Content
                      | "301"   ; Moved Permanently
                      | "302"   ; Moved Temporarily
                      | "304"   ; Not Modified
                      | "400"   ; Bad Request
                      | "401"   ; Unauthorized
                      | "403"   ; Forbidden
                      | "404"   ; Not Found
                      | "500"   ; Internal Server Error
                      | "501"   ; Not Implemented
                      | "502"   ; Bad Gateway
                      | "503"   ; Service Unavailable
                      | extension-code

       extension-code = 3DIGIT

       Reason-Phrase  = *<TEXT, excluding CR, LF>

HTTP status codes are extensible, but the above codes are the only ones generally recognized in current practice. HTTP applications are not required to understand the meaning of all registered status codes, though such understanding is obviously desirable. However, applications must understand the class of any status code, as indicated by the first digit, and treat any unrecognized response as being equivalent to the x00 status code of that class, with the exception that an unrecognized response must not be cached. For example, if an unrecognized status code of 431 is received by the client, it can safely assume that there was something wrong with its request and treat the response as if it had received a 400 status code. In such cases, user agents should present to the user the entity returned with the response, since that entity is likely to include human-readable information which will explain the unusual status.

#### 6.2 Response Header Fields
The response header fields allow the server to pass additional information about the response which cannot be placed in the Status-Line. These header fields give information about the server and about further access to the resource identified by the Request-URI.

       Response-Header = Location                ; Section 10.11
                       | Server                  ; Section 10.14
                       | WWW-Authenticate        ; Section 10.16

Response-Header field names can be extended reliably only in combination with a change in the protocol version. However, new or experimental header fields may be given the semantics of response header fields if all parties in the communication recognize them to be response header fields. Unrecognized header fields are treated as Entity-Header fields. 


## METHODS
### D.1.2 DELETE
The DELETE method requests that the origin server delete the resource identified by the Request-URI.

### 8.1 GET
The GET method means retrieve whatever information (in the form of an entity) is identified by the Request-URI. If the Request-URI refers to a data-producing process, it is the produced data which shall be returned as the entity in the response and not the source text of the process, unless that text happens to be the output of the process.

The semantics of the GET method changes to a "conditional GET" if the request message includes an If-Modified-Since header field. A conditional GET method requests that the identified resource be transferred only if it has been modified since the date given by the If-Modified-Since header, as described in Section 10.9. The conditional GET method is intended to reduce network usage by allowing cached entities to be refreshed without requiring multiple requests or transferring unnecessary data. 

### 8.3 POST
The POST method is used to request that the destination server accept the entity enclosed in the request as a new subordinate of the resource identified by the Request-URI in the Request-Line. POST is designed to allow a uniform method to cover the following functions:

    Annotation of existing resources;
    Posting a message to a bulletin board, newsgroup, mailing list, or similar group of articles;
    Providing a block of data, such as the result of submitting a form [3], to a data-handling process;
    Extending a database through an append operation. 

The actual function performed by the POST method is determined by the server and is usually dependent on the Request-URI. The posted entity is subordinate to that URI in the same way that a file is subordinate to a directory containing it, a news article is subordinate to a newsgroup to which it is posted, or a record is subordinate to a database.

A successful POST does not require that the entity be created as a resource on the origin server or made accessible for future reference. That is, the action performed by the POST method might not result in a resource that can be identified by a URI. In this case, either 200 (ok) or 204 (no content) is the appropriate response status, depending on whether or not the response includes an entity that describes the result.

If a resource has been created on the origin server, the response should be 201 (created) and contain an entity (preferably of type "text/html") which describes the status of the request and refers to the new resource.

A valid Content-Length is required on all HTTP/1.0 POST requests. An HTTP/1.0 server should respond with a 400 (bad request) message if it cannot determine the length of the request message's content.

Applications must not cache responses to a POST request because the application has no way of knowing that the server would return an equivalent response on some future request. 