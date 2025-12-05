# Webserv
Minimal HTTP/1.1 server in C++98 with CGI, uploads, autoindex, redirects, and configurable virtual hosts.

## Highlights
- Serve static files with GET, handle POST uploads and DELETE, per-location autoindexing and body limits.
- CGI execution through an extension-to-interpreter map (e.g. `.py -> /usr/bin/python3`) with pipe-based streaming and timeouts.
- Multiple servers/ports and virtual hosts selected via Host header; configurable redirects and custom error pages.
- Upload handling with per-location stores and permission checks; optional directory listings.
- Poll-based event loop with client inactivity timeout (30s) and CGI timeout (10s).

## Build and Run
- Requirements: clang++/g++ with C++98, make, POSIX sockets (Linux or macOS).
- Build: `make`
- Run: `./webserv [path/to/config]` (defaults to `config/default.conf`).
- Quick try: `curl http://127.0.0.1:8080/` or `curl -X POST -d 'hello' http://127.0.0.1:8080/echo`.

## Configuration
Syntax is nginx-like: `server { ... }` blocks containing optional `location /path { ... }` blocks. Every directive ends with `;`.

Server directives:
- `listen <port>;` one or more ports (required)
- `host <ipv4>;` bind address, defaults to 0.0.0.0
- `server_name <name>;` virtual host name
- `root <path>;` document root (default `./www`)
- `index <file> [file...];` default index files (default `index.html`)
- `methods GET POST DELETE;` allowed methods (default GET)
- `autoindex on|off;` directory listing (default off)
- `client_max_body_size <bytes>;` max request body (default 1048576)
- `error_page <code> <path>;` custom error page for a status code
- `cgi .ext /path/to/interpreter;` map an extension to an interpreter
- `return <code> <target>;` redirect/early return

Location directives inherit unspecified values from the parent server and support:
- `root`, `index`, `methods`, `autoindex`, `client_max_body_size`, `cgi`, `return`
- `upload_enable on|off;`
- `upload_store <dir>;`


## Static Files and Uploads
- Static responses are resolved under the `root`; if the path is a directory, index files are tried, then optional autoindex is generated.
- For a location with `upload_enable on; upload_store <dir>;`, POST bodies are written to `<root>/<uploadStore>/upload_<timestamp>`; a 201 Created is returned with a Location header.

## CGI
- A request is routed to CGI when its path ends with a configured extension and the script is readable inside the location root.
- Environment variables include `REQUEST_METHOD`, `SCRIPT_FILENAME`, `QUERY_STRING`, `SERVER_PROTOCOL`, `SERVER_NAME`, `SERVER_PORT`, `HTTP_HOST`, `CONTENT_TYPE`, `CONTENT_LENGTH`, `DOCUMENT_ROOT`, `PATH_INFO`, `REQUEST_URI`, and `REDIRECT_STATUS`.
- The request body is streamed to the CGI stdin; stdout is parsed for headers and body. On timeout or error, the server replies with a 504/502.

## Project Layout
- `src/`, `inc/`: server implementation
- `config/`: example configs and edge cases
- `www/`: default web root and CGI scripts under `www/cgi-bin`
- `tests/`, `Testers/`: project tests and tooling
- `bin/`, `webserv`: build outputs

## Troubleshooting
- If the server exits early, check config validity; `ConfigFile::validateConfigPath` rejects unreadable/missing files.
- Ensure interpreters used in `cgi` directives are executable and upload stores are writable. A timed-out CGI produces a 504 response.***
