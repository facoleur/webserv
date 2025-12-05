// ServerRequest.cpp

#include "Config.hpp"
#include "Enums.hpp"
#include "Logger.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include <string>
#include <sys/types.h>

void Server::add_bad_request_to_queue(ClientContext& context) {
    Request req(context, context.pfd.fd);
    req.setStatusCode(BAD_REQUEST);
    context.requests.push(req);
}

void Server::handleInvalidRequest(ClientContext& context, Response& res, struct pollfd& pfd) {
    std::string reasonPhrase(ReasonPhrase::get(res.getStatusCode()));
    LOG_DEBUG("handle_requests() exiting with error: " + reasonPhrase);

    context.write_buffer.append(res.serialize());
    std::queue<Request> empty;
    std::swap(context.requests, empty);
    context.close_after_responses = true;
    pfd.events                    = POLLOUT;
}

void Server::handle_requests(ClientContext& context, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {

    RequestRouter router;

    LOG_DEBUG("handle_requests: " + toString(context.requests.size()) + " requests in queue");

    while (!context.requests.empty()) {
        Request&    req        = context.requests.front();
        std::string hostHeader = req.getHeader(HOST);

        LOG_DEBUG("- handling request:");

        // obtain the right config based on Host header
        const ServerConfig& config = getServerConfig(context, _config, hostHeader);

        // OLD CODE
        // Response res;
        // res = router.route(req, config);
        // if (res.isError()) {
        //     handleInvalidRequest(context, res);
        //     break;
        // }
        // context.write_buffer.append(res.serialize());
        // context.requests.pop();
        // (void)nfds;

        // NEW CODE
        // process the request
        Response res;
        if (req.getState() == PENDING) {
            LOG_DEBUG("handle_requests(): PENDING");
            res = router.route(req, config);
            if (res.isError()) {
                handleInvalidRequest(context, res, pfds[i]);
                break;
            }
            if (res.getMustLaunchCgi())
                req.setState(CGI_START);
            else {
                context.write_buffer.append(res.serialize());
                context.requests.pop();
                pfds[i].events = POLLOUT;
                continue;
            }
        }
        if (req.getState() == CGI_START) {
            LOG_DEBUG("handle_requests(): CGI_START");
            if (launchCgi(req, pfds, nfds) != 0) {
                req.setStatusCode(BAD_GATEWAY);
                res = router.route(req, config);
                handleInvalidRequest(context, res, pfds[i]);
                break;
            }
            req.setState(CGI_STREAMING);
            break;
        }
        if (req.getState() == CGI_STREAMING) {
            LOG_DEBUG("handle_requests(): CGI_STREAMING");
            break;
        }
        if (req.getState() == CGI_DONE) {
            LOG_DEBUG("handle_requests(): CGI_DONE");
            res = router.generateResponseFromCgiOutput(req, res, req.cgiInfo.getOutput());
            if (res.isError()) {
                handleInvalidRequest(context, res, pfds[i]);
                break;
            }
            context.write_buffer.append(res.serialize());
            context.requests.pop();
            LOG_DEBUG("CGI_DONE is over");
            pfds[i].events  = POLLOUT;
            pfds[i].revents = 0;
        }
    }
}

// handles partial request i.e. unfinished request but no more POLLIN revents (see Server::run() loop)
// this is a case of bad request
void Server::handlePartialRequest(ClientContext& context, int i, struct pollfd (&pfds)[MAX_EVENTS], int& nfds) {
    add_bad_request_to_queue(context); // the request was partial and not in the queue
    LOG_DEBUG("handlePartialRequest: added bad request to queue");
    handle_requests(context, i, pfds, nfds);
    context.req_parser.setState(REQ_PARSE_COMPLETE);
}
