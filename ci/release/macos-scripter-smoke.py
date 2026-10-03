"""Verify the packaged macOS Scripter can load bundled Python extensions."""

import decimal
import lzma
import os
import sqlite3
import ssl
import sys


expected_prefix = os.environ.get("APSCRIBE_EXPECTED_PYTHON_PREFIX")
if expected_prefix and os.path.realpath(sys.prefix) != os.path.realpath(expected_prefix):
    raise RuntimeError(f"Scripter used external Python: {sys.prefix}")

result_path = os.environ.get("APSCRIBE_SMOKE_RESULT")
if result_path:
    with open(result_path, "w", encoding="utf-8") as result:
        result.write(sys.prefix + "\n")

print("APSCRIBE_SCRIPTER_SMOKE_OK")
