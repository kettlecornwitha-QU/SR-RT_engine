#!/usr/bin/env python3
from __future__ import annotations

import struct
import zlib
from pathlib import Path


def _read_token(stream) -> bytes:
    token = bytearray()
    while True:
        ch = stream.read(1)
        if not ch:
            return bytes(token)
        if ch == b"#":
            stream.readline()
            continue
        if not ch.isspace():
            token.append(ch[0])
            break
    while True:
        ch = stream.read(1)
        if not ch or ch.isspace():
            break
        token.append(ch[0])
    return bytes(token)


def read_ppm(path: Path) -> tuple[int, int, list[tuple[float, float, float]]]:
    with path.open("rb") as f:
        magic = _read_token(f)
        if magic not in {b"P3", b"P6"}:
            raise ValueError(f"Unsupported PPM format in {path}")
        width = int(_read_token(f))
        height = int(_read_token(f))
        max_value = int(_read_token(f))
        if max_value <= 0:
            raise ValueError(f"Invalid max value in {path}")
    scale = 1.0 / float(max_value)
    pixels = []
    if magic == b"P6":
        with path.open("rb") as f:
            _ = _read_token(f)
            _ = _read_token(f)
            _ = _read_token(f)
            _ = _read_token(f)
            raw = f.read(width * height * 3)
            if len(raw) != width * height * 3:
                raise ValueError(f"Unexpected PPM size in {path}")
        for i in range(0, len(raw), 3):
            pixels.append((raw[i] * scale, raw[i + 1] * scale, raw[i + 2] * scale))
    else:
        with path.open("rb") as f:
            _ = _read_token(f)
            _ = _read_token(f)
            _ = _read_token(f)
            _ = _read_token(f)
            values = []
            expected = width * height * 3
            while len(values) < expected:
                tok = _read_token(f)
                if not tok:
                    break
                values.append(int(tok))
            if len(values) != expected:
                raise ValueError(f"Unexpected ASCII PPM size in {path}")
        for i in range(0, len(values), 3):
            pixels.append((values[i] * scale, values[i + 1] * scale, values[i + 2] * scale))
    return width, height, pixels


def write_pfm(path: Path, width: int, height: int, pixels: list[tuple[float, float, float]]) -> None:
    with path.open("wb") as f:
        f.write(f"PF\n{width} {height}\n-1.0\n".encode("ascii"))
        for y in range(height - 1, -1, -1):
            row = pixels[y * width:(y + 1) * width]
            for r, g, b in row:
                f.write(struct.pack("fff", float(r), float(g), float(b)))


def read_pfm(path: Path) -> tuple[int, int, list[tuple[float, float, float]]]:
    with path.open("rb") as f:
        magic = f.readline().strip()
        if magic != b"PF":
            raise ValueError(f"Unsupported PFM format in {path}")
        dims = f.readline().strip().split()
        if len(dims) != 2:
            raise ValueError(f"Invalid PFM dimensions in {path}")
        width, height = int(dims[0]), int(dims[1])
        scale = float(f.readline().strip())
        little_endian = scale < 0
        data = f.read(width * height * 3 * 4)
        if len(data) != width * height * 3 * 4:
            raise ValueError(f"Unexpected PFM size in {path}")
    fmt = ("<" if little_endian else ">") + ("f" * width * height * 3)
    floats = struct.unpack(fmt, data)
    pixels = []
    for y in range(height - 1, -1, -1):
        row_start = y * width * 3
        for x in range(width):
            i = row_start + x * 3
            pixels.append((floats[i], floats[i + 1], floats[i + 2]))
    return width, height, pixels


def write_ppm(path: Path, width: int, height: int, pixels: list[tuple[float, float, float]]) -> None:
    with path.open("wb") as f:
        f.write(f"P6\n{width} {height}\n255\n".encode("ascii"))
        buf = bytearray()
        for r, g, b in pixels:
            buf.extend(
                [
                    max(0, min(255, int(round(r * 255.0)))),
                    max(0, min(255, int(round(g * 255.0)))),
                    max(0, min(255, int(round(b * 255.0)))),
                ]
            )
        f.write(buf)


def write_png(path: Path, width: int, height: int, pixels: list[tuple[float, float, float]]) -> None:
    def chunk(tag: bytes, payload: bytes) -> bytes:
        return (
            struct.pack(">I", len(payload))
            + tag
            + payload
            + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)
        )

    raw = bytearray()
    for y in range(height):
        raw.append(0)
        row = pixels[y * width:(y + 1) * width]
        for r, g, b in row:
            raw.extend(
                [
                    max(0, min(255, int(round(r * 255.0)))),
                    max(0, min(255, int(round(g * 255.0)))),
                    max(0, min(255, int(round(b * 255.0)))),
                ]
            )

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    idat = zlib.compress(bytes(raw), level=6)
    png = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", ihdr)
        + chunk(b"IDAT", idat)
        + chunk(b"IEND", b"")
    )
    with path.open("wb") as f:
        f.write(png)
