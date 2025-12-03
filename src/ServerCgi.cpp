// ServerCgi.cpp

#include <signal.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "CGI.hpp"
#include "Enums.hpp"
#include "Request.hpp"
#include "RequestRouter.hpp"
#include "Server.hpp"
#include "Webserv.hpp"

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
    close(fd);
    removePollEntry(fd, pfds, nfds);
    _cgiFdMap.erase(it);

    if (it->second.role == CGI_STDIN)
        info->setWriteFd(-1);
    else
        info->setReadFd(-1);
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
    request.cgiInfo.exists = true; // CGI was started

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
    } else
        request.cgiInfo.setCgiPID(pid);

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
    if (info->getWriteFd() >= 0)
        cleanUpCgiFd(info->getWriteFd(), pfds, nfds);
    if (info->getReadFd() >= 0)
        cleanUpCgiFd(info->getReadFd(), pfds, nfds);
    info->exists = false;
}

void Server::handleCgiError(const int fd, struct pollfd (&pfds)[MAX_EVENTS], int& nfds, statusCode statusCode) {
    if (!_cgiFdMap[fd].cgiInfo)
        return;

    Request* req    = _cgiFdMap[fd].cgiInfo->getRequest();
    pid_t    cgiPID = _cgiFdMap[fd].cgiInfo->getCgiPID();

    // CGI handling
    cleanUpBothCgiFds(fd, pfds, nfds);
    terminateCgiProcess(cgiPID);

    // request handling
    req->setStatusCode(statusCode);
    req->setState(CGI_DONE);
    DEBUG_LOG("handleCgiError(): done");
}

int Server::waitForCgiTermination(pid_t pid, Request& req) {
    (void)req;
    int status = 0;
    int ret    = waitpid(pid, &status, WNOHANG);

    if (ret == -1) // If an error is detected or a caught signal aborts the call, a value of -1 is returned and
                   // errno is set to indicate the error.
                   // tear down the CGI state
        return -1; // handle ECHILD ? i.e. no existing CGI ? "The calling process has no existing unwaited-for child
    // processes."
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return -1;
    // if (ret == 0)    // child still running, leave the pipes registered and keep enforcing timeouts
    //                  // do nothing, continue server ?
    //     if (ret > 0) // child exited => clean up pipes, parse buffered output, transition the request to CGI_DONE (or
    //     error
    //                  // if !WIFEXITED/WEXITSTATUS != 0)
    return 0;
}
