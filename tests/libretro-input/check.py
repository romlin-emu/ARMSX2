#!/usr/bin/env python3
"""Compile and exercise the production libretro input staging helper."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as temp:
    binary = Path(temp) / "check"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-pthread",
                    "-I", str(root / "pcsx2-libretro"), "-I", str(root / "3rdparty/libretro"),
                    str(root / "tests/libretro-input/check.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("PASS: libretro input selection and staging")
