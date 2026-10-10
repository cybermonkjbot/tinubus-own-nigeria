"""Small serial client for Epic's native HTTP MCP server, not an MCP server.

The default URL is the private Cribtest11 SSH tunnel on the Mac. Discover the
native schemas before calling tools. A failed request is never retried.
"""

import argparse
import json
import sys
from urllib.parse import urlsplit
from urllib.request import Request, build_opener, ProxyHandler


class UnrealMcp:
    def __init__(self, url):
        parsed = urlsplit(url)
        if parsed.scheme != "http" or parsed.hostname not in {"127.0.0.1", "localhost", "::1"}:
            raise ValueError("Use a loopback URL, through SSH when the editor is remote")
        self.url = url
        self.session = None
        self.version = None
        self.request_id = 0
        self.opener = build_opener(ProxyHandler({}))

    def request(self, method, params=None, notification=False):
        payload = {"jsonrpc": "2.0", "method": method}
        if params is not None:
            payload["params"] = params
        if not notification:
            self.request_id += 1
            payload["id"] = self.request_id
        headers = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}
        if self.session:
            headers["Mcp-Session-Id"] = self.session
        if self.version:
            headers["MCP-Protocol-Version"] = self.version
        request = Request(self.url, data=json.dumps(payload).encode(), headers=headers)
        with self.opener.open(request, timeout=60) as response:
            self.session = response.headers.get("Mcp-Session-Id", self.session)
            if notification or response.status == 202:
                return None
            if "text/event-stream" in response.headers.get("Content-Type", ""):
                data = []
                size = 0
                for line in response:
                    size += len(line)
                    if size > 16 * 1024 * 1024:
                        raise RuntimeError("MCP response exceeded 16 MiB")
                    line = line.decode().rstrip("\r\n")
                    if line.startswith("data:"):
                        data.append(line[5:].lstrip())
                    elif not line and data:
                        message = json.loads("\n".join(data))
                        data = []
                        if message.get("id") == payload["id"]:
                            break
                else:
                    raise RuntimeError("MCP stream ended without a matching response")
            else:
                raw = response.read(16 * 1024 * 1024 + 1)
                if len(raw) > 16 * 1024 * 1024:
                    raise RuntimeError("MCP response exceeded 16 MiB")
                message = json.loads(raw)
            if message.get("id") != payload["id"]:
                raise RuntimeError("MCP response ID mismatch")
            if "error" in message:
                raise RuntimeError(json.dumps(message["error"]))
            return message["result"]

    def initialize(self):
        result = self.request("initialize", {
            "protocolVersion": "2025-03-26",
            "capabilities": {},
            "clientInfo": {"name": "baddie-unreal-python-client", "version": "1.0"},
        })
        self.version = result["protocolVersion"]
        self.request("notifications/initialized", notification=True)
        return result

    def close(self):
        if self.session:
            request = Request(self.url, method="DELETE", headers={
                "Mcp-Session-Id": self.session,
                "MCP-Protocol-Version": self.version,
            })
            try:
                with self.opener.open(request, timeout=5):
                    pass
            except OSError:
                pass


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", default="http://127.0.0.1:18000/mcp")
    parser.add_argument("--method", choices=["initialize", "tools/list", "tools/call"], default="tools/list")
    parser.add_argument("--params", default="{}", help="JSON object; use - to read stdin")
    args = parser.parse_args()
    params = json.load(sys.stdin) if args.params == "-" else json.loads(args.params)
    client = UnrealMcp(args.url)
    try:
        initialized = client.initialize()
        result = initialized if args.method == "initialize" else client.request(args.method, params)
        print(json.dumps(result, indent=2))
        if result.get("isError"):
            sys.exit(1)
    finally:
        client.close()


if __name__ == "__main__":
    main()
