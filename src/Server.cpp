// Server.cpp

#include "Server.hpp"
#include "RequestParser.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include <arpa/inet.h>
#include <cctype>
#include <exception>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>

namespace {

std::string methodToString(requestMethod method) {
    switch (method) {
        case GET:
            return "GET";
        case POST:
            return "POST";
        case DELETE:
            return "DELETE";
        default:
            return "UNKNOWN";
    }
}

std::string trimString(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && (value[start] == ' ' || value[start] == '\t' || value[start] == '\r'))
        ++start;
    size_t end = value.size();
    while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r'))
        --end;
    return value.substr(start, end - start);
}

std::string toLower(const std::string& str) {
    std::string lowered = str;
    for (size_t i = 0; i < lowered.size(); ++i) {
        lowered[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[i])));
    }
    return lowered;
}

bool isSubPath(const std::string& root, const std::string& candidate) {
    if (root.empty())
        return true;
    if (candidate.size() < root.size())
        return false;
    if (candidate.compare(0, root.size(), root) != 0)
        return false;
    if (candidate.size() == root.size())
        return true;
    char last = root[root.size() - 1];
    if (last == '/')
        return true;
    return candidate[root.size()] == '/';
}

} // namespace

std::ostream& operator<<(std::ostream& os, struct pollfd pfd) {
    os << "fd: " << pfd.fd << std::endl;
    os << "events: " << pfd.events << std::endl;
    os << "revents: " << pfd.revents << std::endl;
    return os;
}

void Server::disconnect_client(int& index, int& client_fd, struct pollfd* pfds, int& nfds,
                               std::map<int, ClientContext>& context) {

    // handle_requests(context[client_fd], client_fd);
    context.erase(client_fd);
    pfds[index] = pfds[nfds - 1];
    index--;
    close(client_fd);
    nfds--;
    std::cout << "client disconnected" << std::endl; // moved to after close() in case close fails
}

Server::Server() {
}

Server::Server(const Config& cfg) : _cfg(cfg) {
}

Server::~Server() {
}

void Server::new_connection() {
}

void Server::existing_connection() {
}

void add_bad_request_to_queue(ClientContext& context) {
    Request req;
    req.setStatusCode(BAD_REQUEST);
    context.requests.push(req);
}

int Server::executeCgi(const ServerConfig& serverConfig, const LocationConfig* locationConfig, const Request& request,
                       const std::string& scriptPath, const std::string& interpreter, std::string& responseBody,
                       std::map<std::string, std::string>& responseHeaders, int& statusCode,
                       std::string& statusMessage) {

    responseBody.clear();
    responseHeaders.clear();
    statusCode    = 200;
    statusMessage = "OK";

    std::vector<std::string> envStorage;
    std::string              protocol = request.getProtocolVersion().empty() ? "HTTP/1.1" : request.getProtocolVersion();
    std::string              host     = request.getHeader(HOST);
    std::string              contentType = request.getHeader(CONTENT_TYPE);
    std::string              contentLength = request.getHeader(CONTENT_LENGTH);
    std::string              documentRoot =
        locationConfig ? locationConfig->root : (serverConfig.root.empty() ? "." : serverConfig.root);

    envStorage.push_back("REQUEST_METHOD=" + methodToString(request.getMethod()));
    envStorage.push_back("SCRIPT_FILENAME=" + scriptPath);
    envStorage.push_back("QUERY_STRING=" + request.getQueryString());
    envStorage.push_back("SERVER_PROTOCOL=" + protocol);
    envStorage.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envStorage.push_back("SERVER_SOFTWARE=webserv");
    envStorage.push_back("REDIRECT_STATUS=200");
    envStorage.push_back("SCRIPT_NAME=" + request.getPath());
    envStorage.push_back("PATH_INFO=" + request.getPath());
    envStorage.push_back("REQUEST_URI=" + request.getPath());
    envStorage.push_back("DOCUMENT_ROOT=" + documentRoot);
    envStorage.push_back("SERVER_NAME=" + (serverConfig.host.empty() ? std::string("localhost") : serverConfig.host));
    if (!serverConfig.listen_ports.empty())
        envStorage.push_back("SERVER_PORT=" + toString(serverConfig.listen_ports[0]));
    if (!host.empty())
        envStorage.push_back("HTTP_HOST=" + host);
    if (!contentType.empty())
        envStorage.push_back("CONTENT_TYPE=" + contentType);
    if (!contentLength.empty())
        envStorage.push_back("CONTENT_LENGTH=" + contentLength);

    int stdinPipe[2];
    int stdoutPipe[2];
    if (pipe(stdinPipe) == -1)
        return -1;
    if (pipe(stdoutPipe) == -1) {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        return -1;
    }

    if (pid == 0) {
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
    }

    close(stdinPipe[0]);
    close(stdoutPipe[1]);

    const std::string& body = request.getBody();
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
    }
    close(stdinPipe[1]);

    std::string output;
    char        buffer[4096];
    ssize_t     bytes = 0;
    while ((bytes = read(stdoutPipe[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, static_cast<size_t>(bytes));
    }
    close(stdoutPipe[0]);
    if (bytes == -1) {
        waitpid(pid, NULL, 0);
        return -1;
    }

    int status = 0;
    if (waitpid(pid, &status, 0) == -1)
        return -1;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return -1;

    std::string::size_type headerEnd = output.find("\r\n\r\n");
    size_t                 delimiter = 4;
    if (headerEnd == std::string::npos) {
        headerEnd = output.find("\n\n");
        delimiter = 2;
    }

    std::string headersBlock;
    if (headerEnd != std::string::npos) {
        headersBlock = output.substr(0, headerEnd);
        responseBody = output.substr(headerEnd + delimiter);
    } else {
        responseBody = output;
    }

    if (!headersBlock.empty()) {
        std::istringstream iss(headersBlock);
        std::string        line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            if (line.empty())
                continue;
            std::string::size_type sep = line.find(':');
            if (sep == std::string::npos)
                continue;
            std::string headerName  = line.substr(0, sep);
            std::string headerValue = trimString(line.substr(sep + 1));
            std::string lowered     = toLower(headerName);
            if (lowered == "status") {
                std::istringstream statusStream(headerValue);
                int               code = 0;
                statusStream >> code;
                if (statusStream && code >= 100 && code <= 599) {
                    statusCode = code;
                    std::string text;
                    std::getline(statusStream, text);
                    text = trimString(text);
                    if (!text.empty())
                        statusMessage = text;
                    else
                        statusMessage.clear();
                }
                continue;
            }
            responseHeaders[headerName] = headerValue;
        }
    }

    return 0;
}

void Server::run() {
    struct pollfd pfds[MAX_EVENTS];
    int           nfds = 0;

    // Create one listening socket per server:port
    std::vector<int> listen_fds;
    if (_cfg) {
        const std::vector<ServerConfig>& servers = _cfg.getServers();
        for (size_t si = 0; si < servers.size(); ++si) {
            const ServerConfig& srv = servers[si];
            for (size_t pi = 0; pi < srv.listen_ports.size(); ++pi) {
                int port = srv.listen_ports[pi];
                int lfd  = socket(AF_INET, SOCK_STREAM, 0);
                if (lfd < 0)
                    continue;
                int opt = 1;
                setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
                fcntl(lfd, F_SETFL, O_NONBLOCK);
                fcntl(lfd, F_SETFD, FD_CLOEXEC);

                struct sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port   = htons(port);
                in_addr_t ip    = INADDR_ANY;
                if (!srv.host.empty()) {
                    in_addr a;
                    if (inet_aton(srv.host.c_str(), &a))
                        ip = a.s_addr;
                }
                addr.sin_addr.s_addr = ip;

                if (bind(lfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                    close(lfd);
                    continue;
                }
                if (listen(lfd, SOMAXCONN) < 0) {
                    close(lfd);
                    continue;
                }
                pfds[nfds].fd             = lfd;
                pfds[nfds].events         = POLLIN;
                pfds[nfds].revents        = 0;
                _listenerToServerIdx[lfd] = si;
                listen_fds.push_back(lfd);
                ++nfds;
                if (nfds >= MAX_EVENTS)
                    break;
            }
            if (nfds >= MAX_EVENTS)
                break;
        }
    }/*  else {
        // Fallback: single listener on 8080 for development, should be removed
        int lfd = socket(AF_INET, SOCK_STREAM, 0);
        fcntl(lfd, F_SETFL, O_NONBLOCK);
        fcntl(lfd, F_SETFD, FD_CLOEXEC);
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(8080);
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
        listen(lfd, SOMAXCONN);
        pfds[nfds].fd = lfd; pfds[nfds].events = POLLIN; pfds[nfds].revents = 0; ++nfds;
    } */

    std::map<int, struct ClientContext> context;

    while (1) {
        DEBUG_LOG("\n***** while loop start *****\n\t- nfds: " + to_string(nfds) +
                  "\n\t- pfds[1].events: " + to_string(pfds[1].events));
        if (context[1].write_buffer.empty()) {
            DEBUG_LOG("\t- write buffer: empty");
        } else {
            DEBUG_LOG("\t- write buffer: not empty");
        }
        int n = poll(pfds, nfds, TIMEOUT);
        DEBUG_LOG("\n(((((POLL)))))");
        if (n < 0) {
            DEBUG_LOG("poll err");
            continue;
        }

        for (int i = 0; i < nfds; i++) {
            DEBUG_LOG("* for loop: i == " + to_string(i) + " *");
            int cfd = pfds[i].fd;
            if (pfds[i].revents & (POLLERR | POLLNVAL)) { //  POLLHUP | => below POLLIN handling
                DEBUG_LOG("disconnect 1: poll() returned POLLERR or POLLNVAL");
                disconnect_client(i, cfd, pfds, nfds, context);
                continue;
            }

            // Accept on any listening socket
            if (_listenerToServerIdx.count(cfd) && (pfds[i].revents & POLLIN)) {
                int new_client_fd = accept(cfd, NULL, NULL);
                if (new_client_fd < 0) {
                    DEBUG_LOG("accept error");
                    continue;
                }
                fcntl(new_client_fd, F_SETFL, O_NONBLOCK);
                pfds[nfds].fd          = new_client_fd;
                pfds[nfds].events      = POLLIN;
                pfds[nfds].revents     = 0;
                context[new_client_fd] = ClientContext();
                DEBUG_LOG("new client connected");
                DEBUG_LOG(pfds[i]);
                context[new_client_fd].server_index = _listenerToServerIdx[cfd];
                nfds++;
                DEBUG_LOG("new client connected on server index " + to_string(context[new_client_fd].server_index));
                continue;
            }
            DEBUG_LOG("passed: if (_listenerToServerIdx.count(cfd) && (pfds[i].revents & POLLIN))");

            if (pfds[i].revents & POLLIN) { /* Reading */
                DEBUG_LOG("POLLIN revents");
                char           tmp[READ_SIZE + 1];
                int            len = read(cfd, tmp, READ_SIZE);
                ClientContext& ctx = context[cfd];
                if (len == 0) { // client closed their send side
                    if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL) {
                        add_bad_request_to_queue(ctx); // the request was partial and not in the queue
                        DEBUG_LOG("handle_requests: len == 0, partial request. added bad request to queue");
                    }
                    handle_requests(context[cfd], pfds[i]);
                    DEBUG_LOG("disconnect 2: read returned 0");
                    disconnect_client(i, cfd, pfds, nfds, context);
                    continue;
                }
                if (len < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) { // No more data available right now - this is normal
                        DEBUG_LOG("EAGAIN - no more data");
                        continue;
                    }
                    DEBUG_LOG("disconnect 3: read error"); // Real error
                    disconnect_client(i, cfd, pfds, nfds, context);
                    continue;
                }
                /* Parsing */
                tmp[len] = '\0';
                ctx.req_parser.feed(tmp, ctx.requests);
                if (ctx.req_parser.getState() == REQ_PARSE_PARTIAL) { /* Need to parse more */
                    DEBUG_LOG("Req partial");
                    continue;
                }
                handle_requests(ctx, pfds[i]);
                if (!ctx.write_buffer.empty()) { // if we have something to write back to the client,
                    pfds[i].events |= POLLOUT;   // add POLLOUT to watched events for the next poll()
                    DEBUG_LOG("events set to |= POLLOUT");
                } else {
                    DEBUG_LOG("POLLOUT not added to events");
                }
                DEBUG_LOG("\"if (pfds[i].revents & POLLIN)\": continuing");
                continue;
            }
            DEBUG_LOG("passed: if revents & POLLIN");

            /* Response handling */
            if (pfds[i].revents & POLLOUT) {
                DEBUG_LOG("POLLOUT revents");
                std::string& buf = context[cfd].write_buffer;
                while (!buf.empty()) {
                    ssize_t sent = write(cfd, buf.data(), buf.size());
                    DEBUG_LOG("written bytes: " + toString(sent));
                    if (sent > 0) {
                        buf.erase(0, sent);
                        continue;
                    }
                    if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        break; // socket buffer full, wait for next POLLOUT
                    }
                    // error or connection closed
                    DEBUG_LOG("disconnect 4: error or connection closed while sending back response");
                    disconnect_client(i, cfd, pfds, nfds, context);
                    break;
                }
                if (buf.empty()) {              // why this condition ?
                    pfds[i].events &= ~POLLOUT; // Stop watching for POLLOUT. ~ : bitwise NOT
                }
                continue;
            }
            DEBUG_LOG("passed: if revents & POLLOUT");

            if (pfds[i].revents & POLLHUP) {
                DEBUG_LOG("disconnect 5 : POLLHUP");
                disconnect_client(i, cfd, pfds, nfds, context);
                continue;
            }
            DEBUG_LOG("passed: if revents & POLLHUP");
            DEBUG_LOG("*** end of for loop ***");
        }
        DEBUG_LOG("\n***** end of while loop *****");
    }
}

requestValidity Server::handle_requests(ClientContext& context, struct pollfd& pfd) {
    RequestRouter router;

    (void)pfd;

    DEBUG_LOG("handle_requests queue size: " + toString(context.requests.size()));
    while (!context.requests.empty()) {
        Request& req = context.requests.front();

        const ServerConfig& config         = _cfg.getServers()[context.server_index];
        const LocationConfig* locationConf = findLocationConfig(req.getPath(), config);
        const LocationConfig  resolvedConf = resolveConfig(config, locationConf);
        const LocationConfig* effectiveLoc = &resolvedConf;

        std::string fullPath;
        try {
            fullPath = router.resolvePath(req, resolvedConf.root);
        } catch (const std::exception&) {
            Response badRequest(BAD_REQUEST);
            context.write_buffer.append(badRequest.serialize());
            context.requests.pop();
            continue;
        }

        bool useCgi = false;
        std::string interpreter;
        if (!resolvedConf.cgi_map.empty()) {
            std::string::size_type dot = fullPath.find_last_of('.');
            if (dot != std::string::npos) {
                std::string ext = fullPath.substr(dot);
                std::map<std::string, std::string>::const_iterator it = resolvedConf.cgi_map.find(ext);
                if (it == resolvedConf.cgi_map.end() && dot + 1 < fullPath.size()) {
                    std::string altExt = fullPath.substr(dot + 1);
                    it                 = resolvedConf.cgi_map.find(altExt);
                }
                if (it != resolvedConf.cgi_map.end()) {
                    useCgi      = true;
                    interpreter = it->second;
                }
            }
        }

        if (useCgi) {
            if (!isSubPath(resolvedConf.root, fullPath)) {
                Response res(FORBIDDEN);
                context.write_buffer.append(res.serialize());
                context.requests.pop();
                continue;
            }
            struct stat st;
            if (stat(fullPath.c_str(), &st) != 0) {
                Response res(NOT_FOUND);
                context.write_buffer.append(res.serialize());
                context.requests.pop();
                continue;
            }
            if (!S_ISREG(st.st_mode) || access(fullPath.c_str(), R_OK) != 0) {
                Response res(FORBIDDEN);
                context.write_buffer.append(res.serialize());
                context.requests.pop();
                continue;
            }

            std::string                             cgiBody;
            std::map<std::string, std::string>      cgiHeaders;
            int                                     statusCode    = 200;
            std::string                             statusMessage = "OK";
            int                                     result        = executeCgi(config, effectiveLoc, req, fullPath,
                                                                             interpreter, cgiBody, cgiHeaders,
                                                                             statusCode, statusMessage);
            if (result != 0) {
                Response res(BAD_GATEWAY);
                context.write_buffer.append(res.serialize());
                context.requests.pop();
                continue;
            }
            if (cgiHeaders.find("Content-Type") == cgiHeaders.end())
                cgiHeaders["Content-Type"] = "text/html";
            if (cgiHeaders.find("Content-Length") == cgiHeaders.end())
                cgiHeaders["Content-Length"] = toString(cgiBody.size());

            std::string statusText = statusMessage;
            if (statusText.empty()) {
                const char* fallback = ReasonPhrase::get(static_cast<enum statusCode>(statusCode));
                statusText           = fallback ? fallback : "OK";
            }

            std::ostringstream httpResponse;
            httpResponse << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
            for (std::map<std::string, std::string>::const_iterator headerIt = cgiHeaders.begin();
                 headerIt != cgiHeaders.end(); ++headerIt) {
                std::string headerNameLower = toLower(headerIt->first);
                if (headerNameLower == "status")
                    continue;
                httpResponse << headerIt->first << ": " << headerIt->second << "\r\n";
            }
            httpResponse << "\r\n" << cgiBody;
            context.write_buffer.append(httpResponse.str());
            context.requests.pop();
            continue;
        }

        Response res = router.route(req, config);
        context.write_buffer.append(res.serialize());
        context.requests.pop();
    }
    pfd.events = POLLOUT;
    DEBUG_LOG("handle_requests() exiting");
    return VALID_REQUEST;
}
