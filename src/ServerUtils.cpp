// ServerUtils.cpp

#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <string>
#include <sys/time.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "Config.hpp"
#include "Logger.hpp"
#include "Server.hpp"

void Server::setPollFd(struct pollfd& pfd, int socketFd, short events, short revents) {
    pfd.fd      = socketFd;
    pfd.events  = events;
    pfd.revents = revents;
}

// similar to part of disconnect_client()
void Server::removePollEntry(int targetFd, struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {
    for (int i = 0; i < nfds; ++i) {
        if (pfds[i].fd == targetFd) {
            pfds[i] = pfds[nfds - 1];
            --nfds;
            return;
        }
    }
}

void Server::disconnect_client(int& index, int& client_fd, struct pollfd (&pfds)[MAX_EVENTS], int& nfds,
                               ContextMap& contextMap) {

    pfds[index] = pfds[nfds - 1];
    index--;
    nfds--;
    contextMap.erase(client_fd);
    close(client_fd);
    LOG_INFO("client disconnected");
}

// Create one listening socket per server:port
std::vector<int> Server::initListenerSockets(struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {
    std::vector<int>                 listen_fds;
    int                              listener;
    struct sockaddr_in               addr;
    in_addr                          a;
    int                              opt;
    const std::vector<ServerConfig>& servers = _config.getServers();

    std::map<std::pair<std::string, int>, std::vector<int> > listenMap;

    for (size_t si = 0; si < servers.size(); si++) {
        const ServerConfig& srv = servers[si];
        for (size_t pi = 0; pi < srv.listen_ports.size(); pi++) {
            int         port = srv.listen_ports[pi];
            std::string ip   = srv.host.empty() ? "0.0.0.0" : srv.host;

            listenMap[std::make_pair(ip, port)].push_back(si);
        }
    }

    for (std::map<std::pair<std::string, int>, std::vector<int> >::iterator it = listenMap.begin();
         it != listenMap.end(); ++it) {

        const std::string&      ipStr         = it->first.first;
        int                     port          = it->first.second;
        const std::vector<int>& serverIndices = it->second;

        listener = socket(AF_INET, SOCK_STREAM, 0);
        if (listener < 0)
            continue;
        opt = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        fcntl(listener, F_SETFL, O_NONBLOCK);
        fcntl(listener, F_SETFD, FD_CLOEXEC);

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(port);

        // ip              = INADDR_ANY;
        // if (!srv.host.empty() && inet_aton(srv.host.c_str(), &a))
        //     ip = a.s_addr;
        // addr.sin_addr.s_addr = ip;

        if (inet_aton(ipStr.c_str(), &a))
            addr.sin_addr = a;
        else
            addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(listener);
            continue;
        }
        if (listen(listener, SOMAXCONN) < 0) {
            close(listener);
            continue;
        }
        setPollFd(pfds[nfds], listener, POLLIN, 0);
        _listenerToServers[listener] = serverIndices;
        listen_fds.push_back(listener);
        ++nfds;
        if (nfds >= MAX_EVENTS)
            break;
    }

    return listen_fds;
}

// retrieves the matching pollfd (if any) for a given fd
int Server::findPollFdIndexFromFd(int fd, struct pollfd (&pfds)[MAX_EVENTS], int nfds) const {
    for (int i = 0; i < nfds; ++i) {
        if (pfds[i].fd == fd)
            return i;
    }
    return -1;
}
