// ServerRequest.cpp

#include "RequestRouter.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include "Webserv.hpp"
#include <sys/_types/_pid_t.h>

void Server::add_bad_request_to_queue(ClientContext& context) {
    Request req(context);
    req.setStatusCode(BAD_REQUEST);
    context.requests.push(req);
}

void Server::handleInvalidRequest(ClientContext& context, Response& res) {
    std::string reasonPhrase(ReasonPhrase::get(res.getStatusCode()));
    DEBUG_LOG("handle_requests() exiting with error: " + reasonPhrase);

    context.write_buffer.append(res.serialize());
    std::queue<Request> empty;
    std::swap(context.requests, empty);
    context.close_after_responses = true;
}

void Server::handle_requests(ClientContext& context, struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {

    RequestRouter router;

    DEBUG_LOG("handle_requests: " + toString(context.requests.size()) + " requests in queue");

    while (!context.requests.empty()) {
        Request&                         req           = context.requests.front();
        std::string                      hostHeader    = req.getHeader(HOST);
        int                              chosenConfig  = -1;
        const std::vector<ServerConfig>& serverConfigs = _config.getServers();

        DEBUG_LOG("- handling request:");
        DEBUG_LOG(req);

        // obtain the right config based on Host header
        for (size_t j = 0; j < context.availableServers.size(); j++) {
            int index = context.availableServers[j];
            std::cout << index << std::endl;
            if (serverConfigs[index].matchServerName(hostHeader)) {
                chosenConfig = index;
                break;
            }
        }
        if (chosenConfig == -1)
            chosenConfig = context.availableServers[0];
        ServerConfig& config = _config.getServers().at(chosenConfig);
        std::cout << "servername: " << config.server_name << std::endl;
        std::cout << "host header: " << req.getHeader(HOST) << std::endl;

        // Response res;
        // res = router.route(req, config);
        // if (res.isError()) {
        //     handleInvalidRequest(context, res);
        //     break;
        // }
        // context.write_buffer.append(res.serialize());
        // context.requests.pop();
        // (void)nfds;

        // process the request
        Response res;
        if (req.getState() == PENDING) {
            DEBUG_LOG("handle_requests(): PENDING");
            res = router.route(req, config);
            if (res.isError()) {
                handleInvalidRequest(context, res);
                break;
            }
            if (res.getMustLaunchCgi())
                req.setState(CGI_START);
            else
                req.setState(DONE);
        }
        if (req.getState() == CGI_START) {
            DEBUG_LOG("handle_requests(): CGI_START");
            if (launchCgi(req, pfds, nfds) != 0) {
                req.setStatusCode(BAD_GATEWAY);
                res = router.route(req, config);
                handleInvalidRequest(context, res);
                break;
            }
            // ?
        }
        if (req.getState() == CGI_STREAMING) {
            DEBUG_LOG("handle_requests(): CGI_STREAMING");
            if (waitForCgiTermination(req.cgiInfo.getCgiPID(), req))
                req.setState(DONE); // ?
            else
                continue; // ?
        }
        if (req.getState() == DONE) {
            DEBUG_LOG("handle_requests(): DONE");
            if (req.cgiInfo.exists)
                router.generateResponseFromCgiOutput(
                    res, req.cgiInfo.getOutput()); // idea; this could be managed elsewhere also
            // any additional steps for CGI requests ?
            context.write_buffer.append(res.serialize());
            context.requests.pop();
        }
    }
    context.pfd.events = POLLOUT;
}

// handles partial request i.e. unfinished request but no more POLLIN revents (see Server::run() loop)
// this is a case of bad request
void Server::handlePartialRequest(ClientContext& context, struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {
    add_bad_request_to_queue(context); // the request was partial and not in the queue
    DEBUG_LOG("handlePartialRequest: added bad request to queue");
    handle_requests(context, pfds, nfds);
    context.req_parser.setState(REQ_PARSE_COMPLETE);
}
