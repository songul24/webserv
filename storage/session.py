#!/usr/bin/env python3
import os, sys
print("Content-Type: text/html\r\n\r", end="")
print("<html><body>")
print("<h1>CGI Session Test</h1>")
print(f"<p>REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'not set')}</p>")
print(f"<p>QUERY_STRING: {os.environ.get('QUERY_STRING', 'not set')}</p>")
print(f"<p>HTTP_COOKIE: {os.environ.get('HTTP_COOKIE', 'not set')}</p>")
print(f"<p>CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'not set')}</p>")
if os.environ.get('REQUEST_METHOD') == 'POST':
    length = int(os.environ.get('CONTENT_LENGTH', 0) or 0)
    body = sys.stdin.read(length)
    print(f"<p>BODY: {body}</p>")
print("</body></html>")
