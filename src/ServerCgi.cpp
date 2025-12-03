// ServerCgi.cpp

#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "CGI.hpp"
#include "Enums.hpp"
#include "Request.hpp"
#include "RequestRouter.hpp"
#include "Server.hpp"

bool Server::isCgiPipe(int listener) const {
    if (_cgiFdMap.find(listener) == _cgiFdMap.end())
        return false;
    return true;
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

int Server::writeToCgi(int (&stdinPipe)[2], int (&stdoutPipe)[2], Request& request) {
    pid_t pid = request.cgiInfo.getCgiPID();

    close(stdinPipe[0]);
    close(stdoutPipe[1]);

    const std::string& body    = request.getBody();
    size_t             written = 0;
    while (written < body.size()) {
        ssize_t chunk = write(stdinPipe[1], body.data() + written, body.size() - written);
        if (chunk <= 0) {
            close(stdinPipe[1]);
            close(stdoutPipe[0]);
            waitpid(pid, NULL, 0);
            return -1;
        }
        written += static_cast<size_t>(chunk);
        request.cgiInfo.setLastActive(time(NULL));
    }
    close(stdinPipe[1]);
    return 0;
}

int Server::readFromCgi(int (&stdoutPipe)[2], Request& request) {
    pid_t       pid = request.cgiInfo.getCgiPID();
    std::string output;
    char        buffer[4096];
    ssize_t     bytes = 0;
    while ((bytes = read(stdoutPipe[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, static_cast<size_t>(bytes));
        request.cgiInfo.setLastActive(time(NULL));
    }
    close(stdoutPipe[0]);
    if (bytes == -1) {
        waitpid(pid, NULL, 0);
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

    CgiPipeInfo readPipeFdInfo;
    readPipeFdInfo.cgiInfo   = &request.cgiInfo;
    readPipeFdInfo.role      = CGI_STDIN;
    _cgiFdMap[readCgiPipeFd] = readPipeFdInfo;

    CgiPipeInfo writePipeFdInfo;
    writePipeFdInfo.cgiInfo   = &request.cgiInfo;
    writePipeFdInfo.role      = CGI_STDOUT;
    _cgiFdMap[writeCgiPipeFd] = writePipeFdInfo;

    // do we have to do something like this ?
    context[new_client_fd].availableServers = _listenerToServers[listener];
}

void Server::cleanUpCgiFds(const int firedFd, struct pollfd (&pfds)[MAX_EVENTS], int& nfds) { // alternative (b)
    CgiFdMap::iterator it = _cgiFdMap.find(firedFd);
    if (it == _cgiFdMap.end() || it->second.cgiInfo == NULL)
        return;

    CgiInfo* info    = it->second.cgiInfo;
    int      writeFd = info->getWriteFd();
    int      readFd  = info->getReadFd();

    if (writeFd >= 0) {
        close(writeFd);
        removePollEntry(writeFd, pfds, nfds);
        _cgiFdMap.erase(writeFd);
        info->setWriteFd(-1);
    }
    if (readFd >= 0) {
        close(readFd);
        removePollEntry(readFd, pfds, nfds);
        _cgiFdMap.erase(readFd);
        info->setReadFd(-1);
    }
    info->exists = false;
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
    //     if (ret > 0) // child exited => clean up pipes, parse buffered output, transition the request to DONE (or
    //     error
    //                  // if !WIFEXITED/WEXITSTATUS != 0)
}