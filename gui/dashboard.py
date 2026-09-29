"""Live web dashboard for the socket demo.

Listens on 127.0.0.1:5556 for the C server (same wire format as the
client/server link) and pushes every message to the browser with
Server-Sent Events. Standard library only.

    python3 gui/dashboard.py        # then open http://127.0.0.1:8000
"""

import json
import socket
import struct
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

FEED_ADDR = ("127.0.0.1", 5556)   # the C server connects here
WEB_ADDR = ("127.0.0.1", 8000)    # the browser connects here
HISTORY = 500                     # messages kept for newly opened pages
INDEX_HTML = (Path(__file__).parent / "index.html").read_bytes()


class Hub:
    """Shared state between the feed thread and the browser threads."""

    def __init__(self):
        self.cond = threading.Condition()
        self.messages = []
        self.seq = 0
        self.server_connected = False

    def publish(self, value, label):
        with self.cond:
            self.seq += 1
            self.messages.append(
                {"seq": self.seq, "t": time.time(), "value": value, "label": label}
            )
            del self.messages[:-HISTORY]
            self.cond.notify_all()

    def set_connected(self, connected):
        with self.cond:
            self.server_connected = connected
            self.cond.notify_all()

    def wait(self, after_seq, was_connected, timeout):
        """Block until there are messages newer than after_seq, the
        connection status changes, or the timeout expires."""
        with self.cond:
            self.cond.wait_for(
                lambda: self.seq > after_seq or self.server_connected != was_connected,
                timeout,
            )
            new = [m for m in self.messages if m["seq"] > after_seq]
            return new, self.server_connected


hub = Hub()


def read_msg(f):
    """Read one message: uint32 float bits, uint8 length, label bytes."""
    head = f.read(5)
    if len(head) < 5:
        return None
    value, n = struct.unpack("!fB", head)
    label = f.read(n)
    if len(label) < n:
        return None
    return value, label.decode("utf-8", "replace")


def feed_loop():
    with socket.create_server(FEED_ADDR) as srv:
        print(f"[dashboard] waiting for the C server on {FEED_ADDR[0]}:{FEED_ADDR[1]}")
        while True:
            conn, _ = srv.accept()
            print("[dashboard] C server connected")
            hub.set_connected(True)
            with conn, conn.makefile("rb") as f:
                while (msg := read_msg(f)) is not None:
                    hub.publish(*msg)
            hub.set_connected(False)
            print("[dashboard] C server disconnected")


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(INDEX_HTML)))
            self.end_headers()
            self.wfile.write(INDEX_HTML)
        elif self.path == "/events":
            self.stream_events()
        else:
            self.send_error(404)

    def stream_events(self):
        self.send_response(200)
        self.send_header("Content-Type", "text/event-stream")
        self.send_header("Cache-Control", "no-cache")
        self.end_headers()

        last_seq, connected = 0, None
        try:
            while True:
                # The first pass returns immediately with the history, because
                # connected=None never equals the real status.
                new, connected = hub.wait(last_seq, connected, timeout=15)
                if new:
                    last_seq = new[-1]["seq"]
                payload = json.dumps({"connected": connected, "messages": new})
                self.wfile.write(f"data: {payload}\n\n".encode())
                self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError):
            pass  # browser tab closed

    def log_message(self, *args):
        pass  # keep the terminal for feed messages


def main():
    sys.stdout.reconfigure(line_buffering=True)
    threading.Thread(target=feed_loop, daemon=True).start()
    web = ThreadingHTTPServer(WEB_ADDR, Handler)
    print(f"[dashboard] open http://{WEB_ADDR[0]}:{WEB_ADDR[1]}")
    try:
        web.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
