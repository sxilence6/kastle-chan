#!/usr/bin/env python3
"""Validate and summarize a Kastle-chan SAMPLES.bin without firmware hardware."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "path",
        nargs="?",
        type=Path,
        default=Path(__file__).parents[1] / "code/src/apps/KastleChan/SAMPLES.bin",
    )
    args = parser.parse_args()
    data = args.path.read_bytes()
    if len(data) < 24:
        raise ValueError("file is too short")

    (magic, declared_size, rate, depth, bank_count, sample_count, scale_count,
     rhythm_count, sequence_length, _, _) = struct.unpack_from("<4sII8B", data, 0)
    if magic != b"k2kc":
        raise ValueError(f"bad magic: {magic!r}")
    if declared_size != len(data):
        raise ValueError(f"declared {declared_size:,} bytes, actual {len(data):,}")
    if data[-4:] != b"ahoj":
        raise ValueError("missing end marker")
    if (bank_count, sample_count) != (6, 32):
        raise ValueError(f"expected 6 x 32 samples, got {bank_count} x {sample_count}")

    offset = 20 + scale_count * 4 + rhythm_count * 4
    total_frames = 0
    banks: list[tuple[str, tuple[int, int, int], float]] = []
    for _bank_index in range(bank_count):
        name_raw, red, green, blue, _ = struct.unpack_from("<8sBBBB", data, offset)
        offset += 12
        bank_frames = 0
        for _sample_index in range(sample_count):
            size, channels, _sample_name, _, _, _ = struct.unpack_from("<IB8s3B", data, offset)
            offset += 16
            if channels != 1 or size % 4:
                raise ValueError("clips must be mono and four-byte aligned")
            offset += size
            bank_frames += size // (depth // 8) // channels
        total_frames += bank_frames
        banks.append((name_raw.rstrip(b"\0").decode("ascii"), (red, green, blue), bank_frames / rate))

    if offset != len(data) - 4:
        raise ValueError(f"parser ended at {offset}, end marker starts at {len(data) - 4}")

    print(
        f"valid k2kc: {bank_count} banks x {sample_count} clips, "
        f"{rate} Hz/{depth}-bit mono, sequence {sequence_length}, {len(data):,} bytes"
    )
    for name, color, duration in banks:
        print(f"  {name:8s} rgb={color!s:16s} {duration:6.1f} s")
    print(f"  total voice duration: {total_frames / rate:.1f} s")


if __name__ == "__main__":
    main()
