### Setup
1. run nginx in `nginx_test/` with the custom config
```
nginx -p $(pwd) -c nginx_test/conf/nginx.conf
```

2. run the CGI server within `nginx_test/`
```
python3 -m http.server --cgi 9001
```

3. chmod +x and run `vsnginx.sh`
