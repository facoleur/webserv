// ServerCgi.cpp

#include <sstream>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "CGI.hpp"
#include "Enums.hpp"
#include "Request.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
#include "Server.hpp"

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
}


int Server::launchCgi(Request& request) {

    // get prepared CGI info
    const std::string              scriptPath  = request.cgiInfo.getScriptPath();
    const std::string              interpreter = request.cgiInfo.getInterpreter();
    const std::vector<std::string> envStorage  = request.cgiInfo.getEnvStorage();

    // setup CGI pipes
    int stdinPipe[2];
    int stdoutPipe[2];
    if (setupCgiPipes(stdinPipe, stdoutPipe) == -1)
        return -1;
    request.cgiInfo.setWriteFd(stdinPipe[1]);
    request.cgiInfo.setReadFd(stdoutPipe[0]);

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

    // write body to CGI
    if (writeToCgi(stdinPipe, stdoutPipe, request) == -1)
        return -1;

    // read output from CGI
    if (readFromCgi(stdoutPipe, request) == -1)
        return -1;

    // wait for CGI termination
    if (waitForCgiTermination(pid, request) == -1)
        return -1;

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