// CgiInfo.cpp

#include "CGI.hpp"

CgiInfo::CgiInfo()
    : exists(false), _request(NULL), _interpreter(), _scriptPath(), _envStorage(), _CgiPID(-1), _lastActive(-1),
      _writeFd(-1), _readFd(-1), _bytesWritten(0), _output() {
}

CgiInfo::~CgiInfo() {
}

// getters
Request* CgiInfo::getRequest(void) const {
    return _request;
}

std::string CgiInfo::getInterpreter(void) const {
    return _interpreter;
}

std::string CgiInfo::getScriptPath(void) const {
    return _scriptPath;
}

std::vector<std::string> CgiInfo::getEnvStorage(void) const {
    return _envStorage;
}

pid_t CgiInfo::getCgiPID(void) const {
    return _CgiPID;
}

int CgiInfo::getLastActive(void) const {
    return _lastActive;
}

int CgiInfo::getWriteFd(void) const {
    return _writeFd;
}

int CgiInfo::getReadFd(void) const {
    return _readFd;
}

int CgiInfo::getBytesWritten(void) const {
    return _bytesWritten;
}

std::string CgiInfo::getOutput(void) const {
    return _output;
}

// setters
void CgiInfo::setRequest(Request& request) {
    _request = &request;
}

void CgiInfo::setInterpreter(const std::string& interpreter) {
    _interpreter = interpreter;
}

void CgiInfo::setScriptPath(const std::string& scriptPath) {
    _scriptPath = scriptPath;
}

void CgiInfo::setEnvStorage(const std::vector<std::string>& envStorage) {
    _envStorage = envStorage;
}

void CgiInfo::setCgiPID(pid_t CgiPID) {
    _CgiPID = CgiPID;
}

void CgiInfo::setLastActive(int lastActive) {
    _lastActive = lastActive;
}

void CgiInfo::setWriteFd(int writeFd) {
    _writeFd = writeFd;
}

void CgiInfo::setReadFd(int readFd) {
    _readFd = readFd;
}

void CgiInfo::setBytesWritten(int bytesWritten) {
    _bytesWritten = bytesWritten;
}

void CgiInfo::appendToOutput(std::string& content) {
    _output += content;
}
