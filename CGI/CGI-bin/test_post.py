#!/usr/bin/env python3
import sys
import os
print("Content-Type: text/html\n")
print("<html><body>")
print("<h1>POST CGI Test</h1>")
body = sys.stdin.read()
print(f"<p>Body length: {len(body)}</p>")
print(f"<p>Content-Type: {os.environ.get('CONTENT_TYPE', 'not set')}</p>")
print("</body></html>")
