from http.server import BaseHTTPRequestHandler, HTTPServer
import json


class Handler(BaseHTTPRequestHandler):
    def _write(self, code, payload):
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(payload).encode())

    def do_GET(self):
        if self.path.startswith('/api/states/cover.living_room_blind'):
            self._write(200, {"state": "open", "attributes": {"current_position": 80}})
        else:
            self._write(404, {"error": "not found"})

    def do_POST(self):
        if self.path.startswith('/api/services/cover/'):
            self._write(200, {"result": "ok"})
        else:
            self._write(404, {"error": "not found"})


if __name__ == '__main__':
    server = HTTPServer(("0.0.0.0", 8123), Handler)
    server.serve_forever()
