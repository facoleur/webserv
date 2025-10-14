Hullow: I am to build an HTTP request parser for a C++98 web server as a school exercise. I have some experience with writing a simple parser for a shell in C, and now I'd like to write a simple recursive descent parser, working ideally with only one pass.

First question:
As input I received a socketFd, I gather. I should read from that socket character by character to build a line, stored in a std::string, until I hit CRLF, correct (in HTTP/1.0 it could be only CR or only LF I think, but I could handle those cases later). Is it as simple as a read() operation ? Or do I need to use recv() (which I am not familiar with) ?

GitHub Copilot: For a C++98 web server school project, you'll want to use `recv()` rather than `read()` for reading from a socket. Here's why:

## Key Differences

- **`recv()`** is specifically designed for sockets and provides socket-specific flags
- **`read()`** works on sockets too (on Unix/Linux), but `recv()` is more portable and explicit

## Basic Usage

```cpp
char buffer[1];
ssize_t bytesRead = recv(socketFd, buffer, 1, 0);
```

The 4th parameter (flags) is usually `0` for standard blocking reads.

## Important Considerations for Your Parser

1. **Reading byte-by-byte is inefficient** - Consider reading into a buffer and processing it:

```cpp
char buffer[4096];
ssize_t bytesRead = recv(socketFd, buffer, sizeof(buffer), 0);
```

2. **Handle partial reads** - TCP is a stream protocol, so you might not get a complete line in one `recv()` call

3. **CRLF vs CR vs LF** - HTTP/1.1 strictly requires `\r\n` (CRLF), but being lenient is fine for a school project

4. **Return value checks**:
   - `> 0`: bytes received
   - `0`: connection closed
   - `< 0`: error (check `errno`)

## Recommended Approach

Keep an internal buffer in your parser class and read chunks at a time, then search for CRLF within the buffered data. This is much more efficient than byte-by-byte reads.

Would you like me to show you a buffered reading approach for your parser?

Hullow: Thank you. Follow-up questions:

(1) You wrote "Keep an internal buffer in your parser class and read chunks at a time". Why read chunks at a time ? Why not simply read until CRLF ?

(2) I was thinking of using a recursive descent parser like in this (see attached) parsing implementation example. I already wrote a Request class to store information about my request. How to articulate the two classes ? I am learning C++ and OOP in general, I have little experience with how to build classes and make them interact. How should things look ? I don't have the main loop of the project at hand (it's being worked on by a team mate) but I'm thinking the Request would be declared there and initialized by my parser, then validated by methods of the Request class, then used by the main loop of the server. I don't know if that make sense and I would like some guidance on how to structure this part of the project. 

GitHub Copilot: ## (1) Why read chunks instead of "read until CRLF"?

Because **you cannot directly "read until CRLF" with socket operations**. Here's why:

- `recv()` doesn't have a "read until delimiter" mode
- You must read into a buffer, *then search for CRLF* in that buffer
- Reading 1 byte at a time with `recv()` means **one system call per byte** = extremely slow
- Reading chunks (e.g., 4096 bytes) means **one system call for many bytes** = much faster

**What actually happens:**
1. Call `recv()` → get up to 4096 bytes (or whatever the socket has ready)
2. Search for `\r\n` in those bytes
3. If found: extract the line, keep remaining bytes for next line
4. If not found: read more bytes, append to buffer, search again

## (2) Class Architecture & Interaction

Here's a clean separation of concerns for C++98:

### Class Responsibilities

````cpp
// RequestParser.hpp

#pragma once

#include "Request.hpp"
#include <string>

/* Parses HTTP request from socket, performing syntactic validation.
 * Populates a Request object with parsed data. */
class RequestParser
{
private:
    std::string _buffer;           // accumulated read data
    size_t      _bufferPos;        // current position in buffer
    int         _socketFd;
    Request*    _request;          // request being built

    // Low-level reading
    bool        readChunk(void);                           // read more data into buffer
    bool        readLine(std::string& line);               // extract line ending with CRLF
    
    // Parsing (syntactic validation)
    bool        parseRequestLine(void);
    bool        parseMethod(std::string const& token);
    bool        parseTarget(std::string const& token);
    bool        parseVersion(std::string const& token);
    bool        parseHeaders(void);
    bool        parseHeader(std::string const& line);
    bool        parseBody(void);

public:
    RequestParser(void);
    ~RequestParser(void);

    // Main entry point: parse socket into Request object
    Request*    parse(int socketFd);
};
````

````cpp
// Request.hpp

#pragma once

#include <string>
#include <map>

enum RequestMethod { GET, POST, DELETE, UNKNOWN };
enum RequestValidity { UNEVALUATED_REQUEST, VALID_REQUEST, INVALID_REQUEST };

/* Stores HTTP request data and performs semantic validation */
class Request
{
private:
    RequestMethod               _method;
    std::string                 _path;
    std::string                 _queryString;
    int                         _protocolVersion;
    std::map<std::string, std::string> _headers;  // Note: use map, not vector
    std::string                 _body;
    RequestValidity             _validity;

    // Semantic validation (business logic)
    bool    isMethodValid(void) const;
    bool    isPathValid(void) const;
    bool    isProtocolVersionValid(void) const;
    bool    isHeadersValid(void) const;
    bool    isBodyValid(void) const;

public:
    Request(void);
    ~Request(void);

    // Setters (called by parser)
    void    setMethod(RequestMethod method);
    void    setPath(std::string const& path);
    void    setQueryString(std::string const& query);
    void    setProtocolVersion(int version);
    void    addHeader(std::string const& key, std::string const& value);
    void    setBody(std::string const& body);

    // Validation
    bool    validate(void);  // performs all semantic checks, sets _validity

    // Getters
    RequestMethod   getMethod(void) const;
    std::string     getPath(void) const;
    bool            hasHeader(std::string const& key) const;
    std::string     getHeader(std::string const& key) const;
    // ... other getters
};
````

### Usage Flow (in your main loop)

````cpp
// Somewhere in your server's main loop:

int clientSocket = accept(serverSocket, ...);

// 1. Parse (syntactic validation)
RequestParser parser;
Request* request = parser.parse(clientSocket);

if (request == NULL)
{
    // Syntactic error during parsing
    sendErrorResponse(clientSocket, 400);
    close(clientSocket);
    return;
}

// 2. Validate (semantic validation)
if (!request->validate())
{
    // Semantic error (e.g., unsupported method, invalid path)
    sendErrorResponse(clientSocket, request);
    delete request;
    close(clientSocket);
    return;
}

// 3. Use the valid request
handleRequest(*request, clientSocket);

delete request;
````

### Key Design Decisions

- **RequestParser**: Owns the parsing logic, returns a *heap-allocated* `Request*` (or NULL on error)
- **Request**: Just a data container with semantic validation
- **Separation**: Parser handles syntax (CRLF, header format), Request handles semantics (method allowed? path safe?)
- **Return NULL on error**: Simple C++98-friendly error handling (no exceptions needed)

Does this architecture make sense for your project?

Hullow: Can you answer my question (2) again, but taking into account my new chat instructions ?

(2) I was thinking of using a recursive descent parser like in this (see attached) parsing implementation example. I already wrote a Request class to store information about my request. How to articulate the two classes ? I am learning C++ and OOP in general, I have little experience with how to build classes and make them interact. How should things look ? I don't have the main loop of the project at hand (it's being worked on by a team mate) but I'm thinking the Request would be declared there and initialized by my parser, then validated by methods of the Request class, then used by the main loop of the server. I don't know if that make sense and I would like some guidance on how to structure this part of the project. 

GitHub Copilot: ## Class Architecture: First Principles

### The Core Question: Who Owns What?

You have **two separate concerns**:

1. **Syntax parsing** (RequestParser): "Does this byte stream follow HTTP grammar rules?"
2. **Data storage + semantic validation** (Request): "Is this a valid request for *our* server?"

### Key Design Principle: Separation of Concerns

**RequestParser should build and return a Request object.**

Why? Because:
- The parser's job is to **transform** raw bytes → structured data
- The Request's job is to **hold** that data and **validate** it semantically
- Your main loop's job is to **use** the validated request

### The Interaction Flow

```
Socket bytes → [RequestParser] → Request* → [Request::validate()] → valid/invalid → [Server uses it]
```

### Ownership & Memory

**Critical decision**: Who allocates the Request?

Option 1: **Parser allocates on heap, returns pointer**
- Parser does `Request* req = new Request();`
- Returns `Request*` (or `NULL` on parse error)
- Caller must `delete req;` when done
- **Pro**: Clear ownership transfer
- **Con**: Manual memory management (but unavoidable in C++98)

Option 2: **Caller provides a Request reference**
- Caller does `Request req;` (stack allocation)
- Parser fills it via `bool parse(int fd, Request& req)`
- **Pro**: No heap allocation
- **Con**: Caller must declare Request before knowing if parse succeeds

**Recommendation for school project**: Option 1 (heap allocation). It's cleaner for error handling.

### Return Values for Recursive Descent

Your attached example returns `null` on parse failure. For C++98 HTTP parsing:

**RequestParser methods should return:**
- `Request*` from main `parse()` method (NULL = syntax error)
- `bool` from internal parse methods (false = doesn't match this production)

**Request validation methods should return:**
- `bool` from `validate()` and helper methods (false = semantic error)

### How Methods Interact

**Parser populates Request via setters:**
1. Parser reads "GET /index.html HTTP/1.1\r\n"
2. Parser calls `request->setMethod(GET)`
3. Parser calls `request->setPath("/index.html")`
4. Parser calls `request->setProtocolVersion(1)` (for 1.1)
5. Repeat for headers and body

**Request validates itself:**
- After parsing completes, main loop calls `request->validate()`
- `validate()` internally calls `isMethodValid()`, `isPathValid()`, etc.
- Each validator checks semantic rules (e.g., "does path contain `..`?")

### Alternative: Return Values vs Exceptions

Your school probably hasn't taught exceptions yet. So use **return codes**:
- `NULL` pointer = parse failed
- `false` = validation failed

Later in industry code, you'd throw exceptions, but for C++98 school: keep it simple.

### Mental Model

Think of it like this:
- **RequestParser** is a **factory** that builds Request objects from raw materials (socket bytes)
- **Request** is a **product** that knows how to validate itself
- Your **main loop** is the **customer** who orders a Request and checks if it's acceptable

The factory doesn't care what the customer does with the product. The product doesn't care how it was built. Clean separation.

### Summary

- Parser builds Request via `new`, returns `Request*` (NULL on error)
- Parser calls Request setters during parsing
- Main loop receives Request, calls `validate()`, uses or rejects it
- Main loop must `delete` the Request when done
- This separation means: parser handles syntax, Request handles semantics, main loop handles routing

Does this mental model clarify how the pieces fit together?

Hullow: Follow-up questions:

(1) so the main loop would look 

(2) why populate the request with setters ? isn't it better to store the results of parsing on the stack (within RequestParaer), and then 

GitHub Copilot: 

Hullow: Follow-up questions:

(1) so the main loop would look like :
```cpp
Request *req = ::RequestParser->parse(socketFd);
if (req->validity == INVALID_REQUEST)
 handle_error(req->status_code);
else
 handle_request(req);
```
?

(2) why populate the request with setters ? isn't it better to store the results of parsing on the stack (within RequestParser logic), and then at the end of the parse() function, if everything is valid, call the Request parameter constructor with all the parameters ? Or maybe that's too heavy code-wise and unwieldy ?

GitHub Copilot: ## (1) Main Loop Structure

Almost, but **you're mixing concerns**. The validity check should be **separate** from parsing:

```
RequestParser parser;
Request* req = parser.parse(socketFd);

if (req == NULL)  // Syntax error during parsing
    handle_parse_error(socketFd, 400);
else if (!req->validate())  // Semantic validation failed
    handle_request_error(socketFd, req);
else
    handle_valid_request(socketFd, req);

delete req;
```

**Why separate?**
- `parse()` returns `NULL` = "malformed HTTP syntax" (e.g., no CRLF, invalid header format)
- `validate()` returns `false` = "well-formed but invalid for our server" (e.g., unsupported method, path traversal)

These are **different failure modes**:
- Syntax error → you can't even build a Request object → return `NULL`
- Semantic error → you built the Request, but it's not allowed → return object, let it validate itself

**Note on your code**: `req->validity == INVALID_REQUEST` assumes validation already happened. Better to **explicitly call** `req->validate()` so it's clear when validation occurs.

---

## (2) Why Setters vs Parameterized Constructor?

Excellent question. Here's the **fundamental tradeoff**:

### Option A: Setters (incremental building)
```
Request* req = new Request();
req->setMethod(GET);
req->setPath("/index.html");
req->addHeader("Host", "localhost");
```

**Why this works better for parsing:**

1. **Parsing is inherently incremental** — you discover data piece by piece:
   - Read request line → now you know method/path
   - Read headers → discover them one at a time
   - Read body → discover it last

2. **Early failure is easy** — if parsing fails halfway, you've built nothing on the heap yet (or you delete the partial Request)

3. **Unknown parameter count** — you don't know how many headers exist until you've read them all. How do you pass "unknown number of headers" to a constructor?

### Option B: Store locally, then construct
```
// Inside RequestParser
requestMethod method;
std::string path;
std::map<std::string, std::string> headers;
std::string body;

// ... parse everything into these locals ...

// At end:
return new Request(method, path, headers, body, ...);
```

**Problems:**

1. **Duplication** — you're maintaining the same data structure twice (once in parser locals, once in Request)
2. **Constructor complexity** — your Request constructor needs 5+ parameters, which is unwieldy
3. **No benefit** — you gain nothing compared to setters, but lose flexibility

**The key insight**: Setters aren't "bad OOP" here. They're **the right tool** because construction is multi-phase. You're building the object **as you discover its contents**.

---

### Alternative: Builder Pattern (overkill for C++98 school project)

In production C++, you might use a RequestBuilder that stores intermediate state and has a `build()` method. But that's just setters with extra steps.

---

## Summary: Mental Model

**Parsing = gradual discovery of structure**

You read bytes → extract tokens → accumulate into object. This is **inherently sequential**. 

Setters match this sequential nature: "I just discovered the method is GET → call `setMethod(GET)`."

A parameterized constructor assumes you **already know all parameters**, which you don't during parsing. You'd have to buffer everything locally first, which is wasteful duplication.

**Rule of thumb**: Use parameterized constructors when all data is available at construction time. Use setters when you're building the object incrementally (like parsing).

Your instinct to question this is good — in other contexts (e.g., creating a Request from known values), a parameterized constructor **would** be better. But for parsing, setters are correct.

Hullow: Follow-up:

(1) why use socketFd as a parameter in each handler in the main loop ? 

(2) if req == NULL, won't delete req; cause problems ?

(3) how should I handle possible errors of my Request class ? There could be a case where req->validate() doesn't work and the value is still UNEVALUATED_REQUEST ?

GitHub Copilot: ## (1) Why `socketFd` as parameter in handlers?

Because **each handler needs to send an HTTP response back to the client**.

When you call `handle_parse_error(socketFd, 400)`, that function must:
- Format an HTTP 400 response
- Call `send(socketFd, ...)` to transmit it back to the client

**The socketFd is your communication channel with the client**. Without it, you can't reply.

**First principle**: The socket is a bidirectional pipe. You read the request from it (in parser), then write the response to it (in handlers). Same file descriptor, opposite direction.

---

## (2) Won't `delete req;` cause problems if `req == NULL`?

**No. `delete NULL;` is explicitly safe in C++.**

From the C++98 standard: deleting a null pointer has no effect. The language guarantees this.

**Why this design?** So you can write cleanup code unconditionally:

```
Request* req = parser.parse(socketFd);
// ... handle req or not ...
delete req;  // always safe, even if req is NULL
```

**Mental model**: `delete` checks "is this pointer non-null?" before deallocating. If null, it does nothing.

**Note**: This is different from C's `free()`, which also has this guarantee, but C++ inherited it explicitly.

---

## (3) How to handle `validate()` errors?

**Current issue in your Request.hpp**: You have `requestValidity _validity` as an attribute, but `validate()` returns `bool`, not the enum.

**Two design choices:**

### Option A: `validate()` sets `_validity` and returns `bool`

```
bool validate(void)
{
    if (!isMethodValid())
    {
        _validity = INVALID_REQUEST;
        return false;
    }
    // ... check other things ...
    
    _validity = VALID_REQUEST;
    return true;
}
```

**Then in main loop:**
```
if (!req->validate())  // also sets _validity internally
{
    // req->getValidity() is now INVALID_REQUEST
    handle_request_error(socketFd, req);
}
```

### Option B: `validate()` only returns enum, doesn't store it

```
requestValidity validate(void)
{
    if (!isMethodValid())
        return INVALID_REQUEST;
    // ...
    return VALID_REQUEST;
}
```

**Then in main loop:**
```
requestValidity validity = req->validate();
if (validity != VALID_REQUEST)
    handle_request_error(socketFd, req, validity);
```

**Recommendation**: **Option A** (return bool, set internal state).

**Why?** Because later, other parts of your code might need to check "is this request valid?" without re-running validation. The `_validity` attribute caches the result.

---

### What if `validate()` itself fails (e.g., crashes)?

**In C++98, you have two options:**

1. **Defensive programming**: Make `validate()` never fail. Check every pointer, handle every edge case. Return `INVALID_REQUEST` for unexpected states.

2. **Document preconditions**: `validate()` assumes the Request was successfully built by the parser. If parser returned a non-NULL Request, it must be structurally sound enough to validate.

**For a school project**: Option 1 is safer. Make `validate()` robust:

```
bool validate(void)
{
    // Even if _validity is already set, re-validate
    // (idempotent operation)
    
    if (!isMethodValid() || !isPathValid() || ...)
    {
        _validity = INVALID_REQUEST;
        return false;
    }
    
    _validity = VALID_REQUEST;
    return true;
}
```

**Crucially**: If `validate()` encounters something it can't check (e.g., uninitialized data), treat that as `INVALID_REQUEST` rather than crashing.

---

## Summary: Mental Model

**Error handling strategy for HTTP parsing:**

1. **Parse failure (syntax)**: Parser returns `NULL` → you can't even build a Request → send 400 Bad Request
2. **Validation failure (semantics)**: Parser returns Request, but `validate()` returns `false` → Request is well-formed but violates rules → send appropriate 4xx error (400, 405, etc.)
3. **Validation implementation**: Make it idempotent and defensive. Assume nothing. Set `_validity` internally, return bool for convenience.

**The `_validity` attribute serves two purposes:**
- Cache: so you don't re-run expensive validation
- State tracking: so other code can query "was this validated?" without calling `validate()` again

This separation (parse → validate → use) matches the HTTP spec's distinction between syntax errors (malformed) and semantic errors (forbidden).

Hullow: Follow-up questions:

(1) regarding validate() errors: why not set the enum AND return it ? Or is this bad practice that creates confusion with the getter ?

(2) could it make sense to build RequestParser as an abstract class with static methods, i.e. a ?tool? (terminology?) class ? Maybe not, because I have to instantiate it because I have to store some things within it during the parsing process, right ?

GitHub Copilot: ## (1) Why not set AND return the enum?

**You absolutely can**. Both designs are valid. The tradeoff:

**Option: Return `bool`, set `_validity` internally**
- **Pro**: Simple true/false interface, matches common validation patterns
- **Pro**: The enum is an implementation detail, hidden from caller
- **Con**: Caller must call `getValidity()` to know *why* it failed (if you distinguish multiple failure types)

**Option: Return `requestValidity`, also set `_validity` internally**
- **Pro**: Single call gives you both the result AND caches it
- **Pro**: More explicit - the return type documents what validation does
- **Con**: Redundant if you always check the return value (why store it too?)

**The key question**: Do you need to distinguish `UNEVALUATED_REQUEST` from `INVALID_REQUEST` later?

**If yes**, return the enum. **If no** (i.e., once validated, you only care valid/invalid), return `bool`.

**Your current design has `UNEVALUATED_REQUEST`**, which suggests you might check "has this been validated yet?" elsewhere in your code. If so, **return the enum AND store it**:

```
requestValidity validate(void)
{
    // ... validation logic ...
    _validity = VALID_REQUEST; // or INVALID_REQUEST
    return _validity;
}
```

**Not bad practice**. The getter returns cached state. The validator computes and caches. They have different purposes.

**Alternative**: If you never need `UNEVALUATED_REQUEST`, remove it from the enum. Then return `bool` and internally set `VALID_REQUEST` or `INVALID_REQUEST`.

**First principle**: The enum exists to track state. If that state is only ever "computed once during validation," you don't need `UNEVALUATED`. If it means "we haven't run validate() yet," keep it and return the enum.

---

## (2) Should RequestParser be a static/utility class?

**No. It needs instance state.**

**Why?** Because parsing is inherently **stateful**:

1. **You accumulate a buffer** as you read chunks from the socket
2. **You track position** within that buffer as you consume tokens
3. **You might need partial state** (e.g., "I've read 500 bytes of a 1000-byte body")

**What you're thinking of** is called a **utility class** or **namespace** in C++ (terminology correction). Examples:
- `std::string` has static methods? No.
- Math utilities: `static double Math::abs(double x)` - no state needed

**RequestParser needs state**, so it must be instantiated:
- `_buffer` to hold accumulated socket data
- `_bufferPos` to track where you are in the buffer
- `_socketFd` (maybe) if you want to encapsulate the socket

**Mental model**: 
- **Utility class/namespace**: pure functions, no state (e.g., string manipulation helpers)
- **Parser object**: holds parsing state, processes input incrementally

**Concrete example of why you need state**:

Imagine you call `recv()` and get:
```
"GET /index.html HTTP/1.1\r\nHost: localho"
```

You've read part of the Host header, but it's incomplete. You must:
1. **Store** `"Host: localho"` somewhere (your `_buffer`)
2. **Remember** you're mid-header
3. Call `recv()` again to get `"st\r\n\r\n"`
4. **Append** to buffer, now you have the complete header

**This is state**. You cannot do this with static methods alone.

---

## Summary: Mental Model

**`_validity` enum with `UNEVALUATED`**: Means you're tracking "has validation happened?" If you need this distinction, return the enum from `validate()`. If you only care pass/fail after calling `validate()`, return `bool` and set the enum internally.

**Parser as object vs utility class**: Parsing is **incremental** - you consume input piece by piece, maintaining state between reads. This requires instance variables. A utility class is for **stateless transformations** (e.g., "convert string to int"). Your parser is a **state machine**, so it needs to be instantiated.

**Rule of thumb**: If your class needs member variables that change during operation, it's not a utility class.

Hullow: Thank you. Follow-up questions:

(1) You wrote:
"The key question: Do you need to distinguish UNEVALUATED_REQUEST from INVALID_REQUEST later?

If yes, return the enum. If no (i.e., once validated, you only care valid/invalid), return bool.

Your current design has UNEVALUATED_REQUEST, which suggests you might check "has this been validated yet?" elsewhere in your code. If so, return the enum AND store it"

My answer is that I don't know, I am building this part of the project and I don't know whether it would make sense to check whether a given request has not been evaluated later on. What does this hinge upon ? Are there effectively cases where a request could be unevaluated ?

(2) In RequestParser, you mentioned "encapsulating socketFd". I don't really know what that means, and I'd like to know

GitHub Copilot: 

Hullow: Thank you. Follow-up questions:

(1) You wrote:
"The key question: Do you need to distinguish UNEVALUATED_REQUEST from INVALID_REQUEST later?

If yes, return the enum. If no (i.e., once validated, you only care valid/invalid), return bool.

Your current design has UNEVALUATED_REQUEST, which suggests you might check "has this been validated yet?" elsewhere in your code. If so, return the enum AND store it"

My answer is that I don't know, I am building this part of the project and I don't know whether it would make sense to check whether a given request has not been evaluated later on. What does this hinge upon ? Are there effectively cases where a request could be unevaluated ?

(2) In RequestParser, you mentioned "encapsulating socketFd". I don't really know what that means, and I'd like to know

GitHub Copilot: ## (1) When would a Request be `UNEVALUATED`?

**The fundamental question**: Does your Request object ever exist without immediately being validated?

**In your current architecture: NO.**

Look at your main loop flow:
```
Request* req = parser.parse(socketFd);
if (req == NULL) ...
else if (req->validate() == INVALID_REQUEST) ...
```

**The Request is validated immediately after parsing**. There's no gap where it exists in an unevaluated state visible to other code.

**When you WOULD need `UNEVALUATED_REQUEST`:**

1. **Lazy validation**: You parse the request, store it somewhere, and validate it later (e.g., in a different function or thread)
2. **Partial validation**: You validate parts incrementally (e.g., validate headers, then later validate body)
3. **Request reuse**: You clear and reuse the same Request object for multiple requests

**Your case**: None of these apply. You parse → validate → use/discard. Linear flow.

**Recommendation**: **Remove `UNEVALUATED_REQUEST`** from your enum. You don't need it.

**Simplified enum:**
```
enum requestValidity
{
    VALID_REQUEST,
    INVALID_REQUEST
};
```

**Then `validate()` returns `requestValidity`** (not bool), sets `_validity`, and you always know the state is one of two values.

**First principle**: Only model states that actually exist in your program's lifetime. If a Request never sits around unevaluated, don't represent that state.

**Alternative mental model**: Initialize `_validity` to `INVALID_REQUEST` in constructor. It stays invalid until `validate()` proves it's valid. No "unevaluated" state needed.

---

## (2) What does "encapsulate socketFd" mean?

**Encapsulation** (core OOP term): Hiding implementation details inside an object.

**Two design choices for your parser:**

### Option A: Pass socketFd to every parse method (no encapsulation)
```
class RequestParser {
    bool parseHeaders(int socketFd);
    bool parseBody(int socketFd);
};
```

**Caller does:**
```
RequestParser parser;
Request* req = parser.parse(socketFd); // socketFd passed once
```

**Inside parse():**
```
parseHeaders(socketFd);  // pass it down
parseBody(socketFd);     // pass it again
```

**socketFd is "external"** - the parser doesn't own it, just uses it.

### Option B: Store socketFd as member (encapsulation)
```
class RequestParser {
private:
    int _socketFd;

public:
    bool parseHeaders(void);  // no parameter, uses _socketFd
    bool parseBody(void);
};
```

**Caller does:**
```
RequestParser parser;
Request* req = parser.parse(socketFd);  // stores it internally
```

**Inside parse():**
```
_socketFd = socketFd;  // store it once
parseHeaders();        // automatically uses _socketFd
parseBody();           // automatically uses _socketFd
```

**socketFd is "internal"** - the parser owns it during parsing.

---

**Which is better for your project?**

**Option A (pass as parameter)** is simpler for C++98 school projects because:
- No member variable to manage
- Clear that the parser doesn't own the socket
- The socket lifetime is managed by the main loop, not the parser

**Option B (encapsulate)** is cleaner if:
- You have many internal methods that all need the socket
- You want to hide the socket from method signatures

**For a beginner project**: **Option A**. Less state to track, clearer ownership.

**Terminology correction**: "Encapsulation" generally means "bundling data with methods that operate on it." Here specifically, it means "storing the socket as a member instead of passing it around."

---

## Summary: Mental Model

**`UNEVALUATED_REQUEST`**: Only needed if a Request can exist without being validated. Your flow is parse → validate immediately, so you don't have unevaluated Requests floating around. Remove it. Initialize to `INVALID_REQUEST`, let `validate()` set it to `VALID_REQUEST` if checks pass.

**Encapsulation of socketFd**: Storing it as `_socketFd` vs passing it as a parameter. Both work. Passing as parameter is simpler for learning - fewer member variables, clearer that the parser doesn't own the socket's lifetime. "Encapsulation" here means "hide implementation detail inside the object" (the socket becomes internal state rather than external dependency).

**Rule of thumb**: Only store something as a member if multiple methods need it AND its lifetime is tied to your object. If you just need it briefly during one function call, pass it as a parameter.