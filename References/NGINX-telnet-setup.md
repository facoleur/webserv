# How to setup NGINX and telnet to conduct tests

Setting up NGINX on macOS
Installation
Using Homebrew (simplest):

This installs NGINX and sets up:

- Executable: /opt/homebrew/bin/nginx (Apple Silicon) or /usr/local/bin/nginx (Intel)
- Config: /opt/homebrew/etc/nginx/nginx.conf (or /usr/local/etc/nginx/)
- Default web root: /opt/homebrew/var/www (or /usr/local/var/www)

Basic Operations
Start NGINX:

```bash
nginx
# or with homebrew services:
brew services start nginx
```

Stop it:
```bash
nginx -s stop
# or:
brew services stop nginx
```

Reload config (after editing):
```bash
nginx -s reload
```

Test config (before reloading):
```bash
nginx -t
```



For Your Webserv Testing
What you'll do:

1. Create a test config similar to your webserv config
2. Send the same request to both servers via telnet/curl
3. Compare responses line-by-line

Example workflow:

```bash
# Start NGINX on port 8080
nginx
# (edit config to listen on 8080, reload)

# Send test request
printf "GET /index.html HTTP/1.0\r\n\r\n" | nc localhost 8080

# Start your server on 8081
./webserv config.conf

# Same request to yours
printf "GET /index.html HTTP/1.0\r\n\r\n" | nc localhost 8081

# Compare outputs
```

Key Config Locations
Find your nginx.conf:
```bash
nginx -t # shows config file path in output
```

The important parts to match with your server:

- listen 8080; - port number
- root /path/to/files; - document root
- error_page 404 /404.html; - error pages
- location / blocks - routing rules

The HTTP Version Caveat
NGINX defaults to HTTP/1.1 behavior:

- Persistent connections (`Connection: keep-alive`)
- Chunked transfer encoding possible
- More headers in responses

To make it behave like HTTP/1.0:
```conf
location / {
    proxy_http_version 1.0;
    # or test with explicit HTTP/1.0 requests
}
```

Or just send HTTP/1.0 requests explicitly:
```conf
curl -0 http://localhost:8080/  # forces HTTP/1.0
```

Summary: Install via Homebrew, edit the config file to match your test scenarios (same ports, routes, error pages), then compare responses using telnet/curl with explicit HTTP/1.0 request lines. The goal isn't identical output—it's verifying your status codes and header semantics are correct for HTTP/1.0, accounting for version differences.