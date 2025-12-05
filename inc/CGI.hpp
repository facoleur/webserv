// CGI.hpp

#pragma once

#include <string>
#include <sys/types.h>
#include <vector>

class Request;

#define CGI_BUFFER_SIZE 4096
#define CGI_TIMEOUT 10

class CgiInfo {
  public:
    CgiInfo();
    ~CgiInfo();

  public:
    bool exists;

    // getters
    Request*                 getRequest(void) const;
    std::string              getInterpreter(void) const;
    std::string              getScriptPath(void) const;
    std::vector<std::string> getEnvStorage(void) const;
    pid_t                    getCgiPID(void) const;
    int                      getLastActive(void) const;
    int                      getWriteFd(void) const;
    int                      getReadFd(void) const;
    int                      getBytesWritten(void) const;
    std::string              getOutput(void) const;

    // setters
    void setRequest(Request&);
    void setInterpreter(const std::string&);
    void setScriptPath(const std::string&);
    void setEnvStorage(const std::vector<std::string>&);
    void setCgiPID(pid_t);
    void setLastActive(int);
    void setWriteFd(int);
    void setReadFd(int);
    void setBytesWritten(int);
    void appendToOutput(std::string&);

  private:
    // launch variables
    Request*                 _request; // the request owning the CGI
    std::string              _interpreter;
    std::string              _scriptPath;
    std::vector<std::string> _envStorage;

    // run variables
    pid_t       _cgiPid;
    int         _lastActive;
    int         _writeFd;
    int         _readFd;
    int         _bytesWritten;
    std::string _output;
};

// BODY REFERENCE
// The “body reference” is just a pointer/offset into the original HTTP request body so you know how much of it has
// already been written to the child. Instead of copying the body into a new buffer, store either a reference to
// Request::getBody() plus an index (bytesWritten) or a lightweight span structure; then each time the CGI stdin
// pipe is writable you write from body.begin() + bytesWritten onward until everything is sent. This is the info CgiInfo
// must keep so the streaming logic knows what remains to transfer. Intuition recap: route() sets up the plan,
// launchCgi() spawns and registers the child, the poll loop advances read/write/waitpid, and only after the child
// finishes do you construct the HTTP response using the buffers tracked inside CgiInfo; the “body reference” is simply
// a way to remember which portion of the original request body still needs to be fed into the CGI stdin.

// A usable CGIState should hold at least:
// child PID,
// stdin/out pipe FDs,
// offsets into the request body (what remains to write),
// buffers for stdout data (until headers parsed),
// parsed header
// map/status info, and
// timestamps for CGI-specific timeouts.
//
// To distinguish pipe roles, store them explicitly (stdinFd,
// stdoutFd) plus booleans like stdinClosed; when you register the FDs with poll,accompany each pollfd index with
// metadata : either keep a std::map<int, CGIState*> where the key is the FD, or maintain a parallel array of
// structs{fd, role, statePtr} so when pfds[i] fires you immediately know which state and which direction to service.

// handleCgi() enforces sandbox rules (path under root, file readable, interpreter known
