#!/bin/sh
cd "$(dirname "$0")" || exit 1
python3 -u -c 'import http.server; server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), http.server.SimpleHTTPRequestHandler); print("Open http://127.0.0.1:%d/" % server.server_address[1], flush=True); server.serve_forever()'
