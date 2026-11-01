#!/usr/bin/env python3
import zlib

APP_BIN = "application.bin"

with open(APP_BIN, "rb") as f:
    data = f.read()

crc = zlib.crc32(data) & 0xFFFFFFFF

print(f"CRC32 = 0x{crc:08X}")
print(f"Size  = {len(data)} bytes")
