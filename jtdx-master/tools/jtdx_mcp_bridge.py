#!/usr/bin/env python3
"""JTDX-F MCP stdio -> loopback Streamable HTTP bridge (Python 3.9+)."""
import argparse
import json
import os
import sys
import urllib.error
import urllib.parse
import urllib.request


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--url', default=os.environ.get('JTDX_MCP_URL', 'http://127.0.0.1:22380/mcp'))
    args = parser.parse_args()
    endpoint = urllib.parse.urlsplit(args.url)
    if (endpoint.scheme != 'http' or endpoint.hostname not in ('127.0.0.1', 'localhost')
            or not endpoint.port or endpoint.path != '/mcp' or endpoint.username
            or endpoint.password or endpoint.query or endpoint.fragment):
        parser.error('URL must be http://127.0.0.1:PORT/mcp or http://localhost:PORT/mcp')
    token = os.environ.get('JTDX_MCP_TOKEN', '')
    if len(token) < 32 or '\r' in token or '\n' in token:
        parser.error('Set JTDX_MCP_TOKEN to the token copied from Settings > MCP')
    # Never use proxy settings or forward credentials to a redirected address.
    client = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect())
    version = '2025-11-25'
    for line in sys.stdin.buffer:
        request = None
        try:
            if len(line) > 65536:
                raise ValueError('Request exceeds 64 KiB')
            request = json.loads(line)
            if not isinstance(request, dict):
                raise ValueError('Expected one JSON-RPC object per line')
            data = json.dumps(request, ensure_ascii=False, separators=(',', ':')).encode('utf-8')
            req = urllib.request.Request(args.url, data=data, headers={
                'Authorization': 'Bearer ' + token,
                'Content-Type': 'application/json',
                'Accept': 'application/json, text/event-stream',
                'MCP-Protocol-Version': version,
            }, method='POST')
            with client.open(req, timeout=15) as response:
                raw = response.read(2097153)
                if len(raw) > 2097152:
                    raise ValueError('Response exceeds 2 MiB')
                if response.status == 202:
                    continue
                result = json.loads(raw)
                if request.get('method') == 'initialize':
                    version = result.get('result', {}).get('protocolVersion', version)
            if 'id' in request:
                print(json.dumps(result, ensure_ascii=False, separators=(',', ':')), flush=True)
        except (ValueError, OSError, urllib.error.URLError) as exc:
            # Log diagnostic only; neither request payloads nor credentials are echoed.
            message = 'JTDX-F connection/protocol error: ' + str(exc)
            print(message, file=sys.stderr, flush=True)
            if isinstance(request, dict) and 'id' in request:
                print(json.dumps({'jsonrpc': '2.0', 'id': request['id'], 'error': {
                    'code': -32000, 'message': message}}, ensure_ascii=False), flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
