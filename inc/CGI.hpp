// CGI.hpp

#pragma once

#define CGI_BUFFER_SIZE 4096

class CGIState {
  public:
    CGIState();
    ~CGIState();

    int  childPID;
    int  stdinPipeFd;
    int  stdoutPipeFd;
    char writeBuffer[CGI_BUFFER_SIZE];
    char readBuffer[CGI_BUFFER_SIZE];
    int  last_activity;
};
