# Status codes

```cpp
enum ErrorCodes
{
	BAD_REQUEST = 400,
	FORBIDDEN = 403,
	NOT_FOUND = 404,
	NOT_ALLOWED = 405, // checked if relevant => yes (also in Kaydoo's)
	INTERNAL_SERVER_ERROR = 500,
	NOT_IMPLEMENTED = 501,
	HTTP_VERSION_NOT_SUPPORTED = 505
};
```

## Claude recommendation
The Strategic Choice: Hardcoded vs. Configurable Error Pages
The Core Trade-off
Hardcoded = Fast implementation, zero edge cases, immediate reliability.

Configurable = More code, more failure modes (missing files, unreadable files, permission errors), but the subject explicitly asks for it.

What The Subject Actually Requires
Look at your subject:

"Your server must have default error pages if none are provided."

"Set up default error pages." (in configuration section)

This means both:

Hardcoded defaults (when config doesn't specify)
Config-specified custom pages (when it does)
The Practical Answer
Hardcode the common ones, make custom ones optional.

Here's why this is the smart path:

Hardcode these essentials:

400 Bad Request
403 Forbidden
404 Not Found
405 Method Not Allowed
500 Internal Server Error
501 Not Implemented
505 HTTP Version Not Supported
These 7 cover 95% of your server's error cases. They're simple HTML strings compiled into your binary—literally zero failure modes.

Make custom pages work from config, but when:

Config specifies /custom404.html but it doesn't exist → fall back to hardcoded
Config specifies nothing → use hardcoded

The Implementation Strategy
Your error page logic becomes a two-level fallback:

Check config: does it specify a custom page for this error code?
Try to read that file

If file doesn't exist/can't be read → use hardcoded default
If config has no custom page → use hardcoded default
Result: Your server never breaks because of missing error pages. The hardcoded ones are your safety net.

The Reliability Argument
What breaks in a webserver?

CGI execution
File permissions
Malformed requests
Resource limits
What should never break?

Sending an error response
If your 500 Internal Server Error page itself causes an error (file not found), you've created an infinite loop scenario. Hardcoded pages eliminate this entire class of bugs.

The Time Argument
Hardcoding 7 HTML strings: 30 minutes.

Building robust file-reading with error handling for custom pages: 2-3 hours, plus debugging edge cases.

Start with hardcoded. Add custom page support later if time permits. Your server works immediately and reliably.

The Subject Compliance
The subject wants:

✅ Default pages → hardcoded ones
✅ Config-specified pages → optional feature you add
You're not violating anything by prioritizing hardcoded defaults. You're following the "must have defaults" requirement literally.

Summary: Hardcode the 7 essential error codes as minimal HTML strings in your binary. This takes minutes, never fails, and satisfies the "default pages" requirement. Configurable custom pages are a bonus feature you add when the core server works. The hardcoded pages serve as fallbacks even when custom pages are configured but fail to load. This two-tier approach maximizes reliability—error handling cannot itself error.

## The NGINX Comparison Meaning
Claude:<br>
It's not telling you which headers to handle. It's telling you how to verify correctness.

Send NGINX a POST without Content-Length. It returns 411 Length Required. Your server should do the same (or 400—both are valid for HTTP/1.0).

NGINX's behavior shows you what correct looks like for edge cases.


## HTML/1.0 specification
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

##### 405 Method Not Allowed (HTTP/1.1)
> The method specified in the Request-Line is not allowed for the resource identified by the Request-URI. The response MUST include an Allow header containing a list of valid methods for the requested resource.

#### Server Error 5xx
> Response status codes beginning with the digit "5" indicate cases in which the server is aware that it has erred or is incapable of performing the request. If the client has not completed the request when a 5xx code is received, it should immediately cease sending data to the server. Except when responding to a HEAD request, the server should include an entity containing an explanation of the error situation, and whether it is a temporary or permanent condition. These response codes are applicable to any request method and there are no required header fields.

##### 500 Internal Server Error
> The server encountered an unexpected condition which prevented it from fulfilling the request.

##### 501 Not Implemented
> The server does not support the functionality required to fulfill the request. This is the appropriate response when the server does not recognize the request method and is not capable of supporting it for any resource.

##### 502 Bad Gateway
> The server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request.

##### 503 Service Unavailable
> The server is currently unable to handle the request due to a temporary overloading or maintenance of the server. The implication is that this is a temporary condition which will be alleviated after some delay.

>    Note: The existence of the 503 status code does not imply that a server must use it when becoming overloaded. Some servers may wish to simply refuse the connection. 

##### 505 HTTP Version Not Supported
> The server does not support, or refuses to support, the HTTP protocol version that was used in the request message. The server is indicating that it is unable or unwilling to complete the request using the same major version as the client, as described in section 3.1, other than with this error message. The response SHOULD contain an entity describing why that version is not supported and what other protocols are supported by that server. 