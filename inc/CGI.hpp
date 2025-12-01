// CGI.hpp

#pragma once

#define CGI_BUFFER_SIZE 4096

class CGIState {
  public:
    CGIState();
    ~CGIState();

    int  childPID;
    int  stdinPipe[2];
    int  stdoutPipe[2];
    char writeBuffer[CGI_BUFFER_SIZE];
    char readBuffer[CGI_BUFFER_SIZE];
    int  last_activity;
};

// A usable CGIState should hold at least: child PID, stdin/out pipe FDs, offsets into the request body (what remains to
// write), buffers for stdout data (until headers parsed), parsed header map/status info, and timestamps for
// CGI-specific timeouts. To distinguish pipe roles, store them explicitly (stdinFd, stdoutFd) plus booleans like
// stdinClosed; when you register the FDs with poll, accompany each pollfd index with metadata: either keep a
// std::map<int, CGIState*> where the key is the FD, or maintain a parallel array of structs {fd, role, statePtr} so
// when pfds[i] fires you immediately know which state and which direction to service.