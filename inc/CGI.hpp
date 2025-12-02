// CGI.hpp

#pragma once

#include <string>
#include <sys/types.h>

#include "Config.hpp"

#define CGI_BUFFER_SIZE 4096

class CgiInfo {
  public:
    CgiInfo();
    ~CgiInfo();

  public:
    bool exists;

    // getters
    std::string              getInterpreter(void) const;
    std::string              getScriptPath(void) const;
    std::vector<std::string> getEnvStorage(void) const;
    pid_t                    getCgiPID(void) const;
    int                      getLastActive(void) const;
    int                      getWriteFd(void) const;
    int                      getReadFd(void) const;
    int                      getBytesWrittenToCgi(void) const;
    std::string              getOutput(void) const;

    // setters
    void setInterpreter(std::string&);
    void setScriptPath(std::string&);
    void setEnvStorage(std::vector<std::string>&);
    void setCgiPID(pid_t);
    void setLastActive(int);
    void setWriteFd(int);
    void setReadFd(int);
    void setBytesWrittenToCgi(int);
    void appendToOutput(std::string&);

  private:
    // launch variables
    std::string              _interpreter;
    std::string              _scriptPath;
    std::vector<std::string> _envStorage;
    std::string&             _bodyReference;
    ServerConfig*            _srvConfigPtr;
    LocationConfig*          _locConfigPtr;

    // run variables
    pid_t       _CgiPID;
    int         _lastActive;
    int         _writeFd;
    int         _readFd;
    int         _bytesWrittenToCgi;
    std::string _output;
};

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