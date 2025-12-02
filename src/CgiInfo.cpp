// CgiInfo.cpp

#include "CGI.hpp"

CgiInfo::CgiInfo()
    : exists(false), _CgiPID(-1), _lastActive(-1), _writeFd(-1), _readFd(-1), _bytesWrittenToCgi(0), _output() {
}

CgiInfo::~CgiInfo() {
}

// getters
std::string CgiInfo::getInterpreter(void) const {
    return _interpreter;
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

int CgiInfo::getBytesWrittenToCgi(void) const {
    return _bytesWrittenToCgi;
}

std::string CgiInfo::getOutput(void) const {
    return _output;
}

// setters
void CgiInfo::setInterpreter(std::string& interpreter) {
    _interpreter = interpreter;
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

void CgiInfo::setBytesWrittenToCgi(int bytesWrittenToCgi) {
    _bytesWrittenToCgi = bytesWrittenToCgi;
}

void CgiInfo::appendToOutput(std::string& output) {
    _output = output;
}
