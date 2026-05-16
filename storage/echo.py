#!/usr/bin/env python3
import os, sys
print("Content-Type: text/plain\r\n\r", end="")
for key, val in sorted(os.environ.items()):
    print(f"{key}={val}")
