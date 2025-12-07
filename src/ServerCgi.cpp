/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCgi.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: francis <francis@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/06 17:41:39 by francis           #+#    #+#             */
/*   Updated: 2025/12/08 00:35:00 by francis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// ServerCgi.cpp

#include <signal.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "CGI.hpp"
#include "Enums.hpp"
#include "Logger.hpp"
#include "Request.hpp"
#include "RequestRouter.hpp"
#include "Server.hpp"
#include "Utils.hpp"

bool Server::isCgiPipe(const int fd) const {
    CgiFdMap::const_iterator it = _cgiFdMap.find(fd);
    if (it == _cgiFdMap.end() || it->second.cgiInfo == NULL)
        return false;
    return true;
}

bool Server::isCGIPipeRole(int listener, CgiPipeRole role) const {
    if (!isCgiPipe(listener))
        return false;
    return _cgiFdMap.at(listener).role == role;
}

int Server::setupCgiPipes(int (&stdinPipe)[2], int (&stdoutPipe)[2]) {
    if (pipe(stdinPipe) == -1)
        return -1;

    int flags = fcntl(stdinPipe[1], F_GETFL);
    if (flags == -1 || fcntl(stdinPipe[1], F_SETFL, flags | O_NONBLOCK) == -1) {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        return -1;
    }

    if (pipe(stdoutPipe) == -1) {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        return -1;
    }

    flags = fcntl(stdoutPipe[0], F_GETFL);
    if (flags == -1 || fcntl(stdoutPipe[0], F_SETFL, flags | O_NONBLOCK) == -1) {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        return -1;
    }
    return 0;
}

// add CGI pipeFds to pfds, to CgiInfo and to CgiMap
void Server::storeCgiPipeFds(const int stdinPipe[2], const int stdoutPipe[2], Request& request,
                             struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {

    int writeCgiPipeFd = stdinPipe[1];
    int readCgiPipeFd  = stdoutPipe[0];
    request.cgiInfo.setWriteFd(writeCgiPipeFd);
    request.cgiInfo.setReadFd(readCgiPipeFd);

    setPollFd(pfds[nfds], writeCgiPipeFd, POLLOUT, 0);
    ++nfds;
    setPollFd(pfds[nfds], readCgiPipeFd, POLLIN, 0);
    ++nfds;

    CgiPipeInfo writePipeFdInfo;
    writePipeFdInfo.cgiInfo   = &request.cgiInfo;
    writePipeFdInfo.role      = CGI_STDIN;
    _cgiFdMap[writeCgiPipeFd] = writePipeFdInfo;

    CgiPipeInfo readPipeFdInfo;
    readPipeFdInfo.cgiInfo   = &request.cgiInfo;
    readPipeFdInfo.role      = CGI_STDOUT;
    _cgiFdMap[readCgiPipeFd] = readPipeFdInfo;
}

void Server::cleanUpCgiFd(int fd, struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {
    if (!isCgiPipe(fd) || fd < 0)
        return;

    CgiFdMap::iterator it   = _cgiFdMap.find(fd);
    CgiInfo*           info = it->second.cgiInfo;
    removePollEntry(fd, pfds, nfds);
    if (it->second.role == CGI_STDIN)
        info->setWriteFd(-1);
    else
        info->setReadFd(-1);
    close(fd);
    _cgiFdMap.erase(it);
}

int Server::launchCgi(Request& request, struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {

    if (nfds > MAX_EVENTS - 2)
        return -1;

    // get the prepared CGI info
    const std::string              scriptPath  = request.cgiInfo.getScriptPath();
    const std::string              interpreter = request.cgiInfo.getInterpreter();
    const std::vector<std::string> envStorage  = request.cgiInfo.getEnvStorage();

    // setup CGI pipes
    int stdinPipe[2];
    int stdoutPipe[2];
    if (setupCgiPipes(stdinPipe, stdoutPipe) == -1)
        return -1;

    // add CGI pipes to all relevant structs
    storeCgiPipeFds(stdinPipe, stdoutPipe, request, pfds, nfds);

    // fork
    pid_t pid = fork();
    if (pid == -1) {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        return -1;
    }

    // set CGI Info
    request.cgiInfo.exists = true;
    request.cgiInfo.setLastActive(time(NULL));

    if (pid == 0) { // child process
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        dup2(stdinPipe[0], STDIN_FILENO);
        dup2(stdoutPipe[1], STDOUT_FILENO);
        close(stdinPipe[0]);
        close(stdoutPipe[1]);

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(interpreter.c_str()));
        argv.push_back(const_cast<char*>(scriptPath.c_str()));
        argv.push_back(NULL);

        std::vector<char*> envp;
        for (size_t i = 0; i < envStorage.size(); ++i) {
            envp.push_back(const_cast<char*>(envStorage[i].c_str()));
        }
        envp.push_back(NULL);

        execve(interpreter.c_str(), &argv[0], &envp[0]);
        _exit(1);
    } else {
        close(stdinPipe[0]);
        close(stdoutPipe[1]);
        request.cgiInfo.setCgiPID(pid);
    }
    return 0;
}

void Server::terminateCgiProcess(pid_t pid) {
    if (pid <= 0)
        return;

    kill(pid, SIGTERM);

    for (int attempt = 0; attempt < 5; ++attempt) {
        int   status = 0;
        pid_t ret    = waitpid(pid, &status, WNOHANG);
        if (ret == pid || ret == -1)
            return;
        usleep(50000); // 50 ms grace slice
    }

    kill(pid, SIGKILL);
    waitpid(pid, NULL, WNOHANG);
}

void Server::cleanUpBothCgiFds(const int fd, struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {
    CgiFdMap::iterator it = _cgiFdMap.find(fd);
    if (it == _cgiFdMap.end() || it->second.cgiInfo == NULL)
        return;

    CgiInfo* info = it->second.cgiInfo;
    if (!info) {
        return;
    }
    if (info->getWriteFd() >= 0)
        cleanUpCgiFd(info->getWriteFd(), pfds, nfds);
    if (info->getReadFd() >= 0)
        cleanUpCgiFd(info->getReadFd(), pfds, nfds);
    info->exists = false;
}

void Server::handleCgiError(const int fd, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, statusCode statusCode) {
    LOG_DEBUG("handleCgiError()");
    LOG_DEBUG("TIMESTAMP: " + toString(time(NULL)));
    if (!_cgiFdMap[fd].cgiInfo)
        return;

    Request* req    = _cgiFdMap[fd].cgiInfo->getRequest();
    pid_t    cgiPID = _cgiFdMap[fd].cgiInfo->getCgiPID();

    // CGI handling
    terminateCgiProcess(cgiPID);
    cleanUpBothCgiFds(fd, pfds, nfds);

    if (req) { // NO_STATUS => no response sent (for silent closing of client)
        // request handling
        req->setStatusCode(statusCode);
        req->setState(CGI_DONE);

        // continue normal request handling so an error response is sent to the client
        int clientFdIndex = findPollFdIndexFromFd(req->getClientFd(), pfds, nfds);
        if (clientFdIndex >= 0)
            handleRequests(req->getClientContext(), clientFdIndex, pfds, nfds);
    }
    LOG_DEBUG("handleCgiError(): done");
}

int Server::waitForCgiTermination(pid_t pid) {
    int status = 0;
    int ret    = waitpid(pid, &status, WNOHANG);

    if (ret == -1)
        return -1;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return -1;
    return 0;
}

// write body to CGI for processing
int Server::writePendingBodyToCgi(int writeFd, struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {
    LOG_DEBUG("writePendingBodyToCgi()");
    CgiPipeInfo& pipe = _cgiFdMap.at(writeFd);
    CgiInfo*     info = pipe.cgiInfo;
    if (!info)
        return -1;
    Request* req = info->getRequest();
    if (!req)
        return -1;

    const std::string& body      = req->getBody();
    int                written   = info->getBytesWritten();
    int                remaining = static_cast<int>(body.size()) - written;

    if (remaining <= 0) {
        cleanUpCgiFd(writeFd, pfds, nfds);
        return 0;
    }

    ssize_t n = write(writeFd, body.data() + written, remaining);
    if (n > 0) {
        info->setBytesWritten(written + static_cast<int>(n));
        info->setLastActive(time(NULL));
        if (info->getBytesWritten() == static_cast<int>(body.size())) {
            cleanUpCgiFd(writeFd, pfds, nfds);
        }
        return 0;
    }

    handleCgiError(writeFd, pfds, nfds, BAD_GATEWAY);
    return -1;
}

// read output from CGI
int Server::readFromCgi(int readFd, struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {
    LOG_DEBUG("readFromCgi()");
    CgiPipeInfo& pipe = _cgiFdMap.at(readFd);
    CgiInfo*     info = pipe.cgiInfo;
    if (!info)
        return -1;
    int      cgiPID = info->getCgiPID();
    Request* req    = info->getRequest();
    char     buf[CGI_BUFFER_SIZE];
    if (!req)
        return -1;

    ssize_t n = read(readFd, buf, sizeof(buf));
    if (n > 0) { // append chunk
        std::string chunk(buf, n);
        info->appendToOutput(chunk);
        info->setLastActive(time(NULL));
        LOG_DEBUG("read " + toString(n) + " bytes; output is now: \n\n{\n" + info->getOutput() + "\n}\n\n");
        return 0;
    }
    if (n == 0) { // EOF: close stdout pipe
        LOG_DEBUG("read 0 bytes; cleaning up Cgi fds");
        cleanUpCgiFd(readFd, pfds, nfds);
        int writeFd = info->getWriteFd();
        if (writeFd >= 0)
            cleanUpCgiFd(writeFd, pfds, nfds);
        if (waitForCgiTermination(cgiPID) != 0)
            req->setStatusCode(BAD_GATEWAY);
        req->setState(CGI_DONE);

        LOG_DEBUG("req->getClientFd() == " + toString(req->getClientFd()));
        int clientFdIndex = findPollFdIndexFromFd(req->getClientFd(), pfds, nfds);
        if (clientFdIndex == -1) {
            LOG_DEBUG("findPollFdIndexFromFd() couldn't find the index for the client fd");
            return -1;
        }
        LOG_DEBUG("findPollFdIndexFromFd() found index " + toString(clientFdIndex));
        handleRequests(req->getClientContext(), clientFdIndex, pfds, nfds);
        return 0;
    }

    LOG_DEBUG("read " + toString(n) + " bytes (read error !)");
    handleCgiError(readFd, pfds, nfds, BAD_GATEWAY);
    return -1;
}
