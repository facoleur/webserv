// CGI.hpp

#pragma once

#include <string>
#include <sys/types.h>
#include <vector>

class Request;

#define CGI_BUFFER_SIZE 4096
#define CGI_TIMEOUT 5

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
