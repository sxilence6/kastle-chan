#!/usr/bin/env python3
"""Generate Kastle-chan's original voice clips and pack SAMPLES.bin."""

from __future__ import annotations

import argparse
import json
import struct
import subprocess
import tempfile
import wave
from pathlib import Path

from kokoro_mlx import KokoroTTS


SCALES = [
    0b000010001001,
    0b101010110101,
    0b010110101101,
    0b111111111111,
    0b101010110101,
    0b001010010101,
    0b000010010001,
]

RHYTHMS = [
    0b1000000010000000,
    0b1000100010001000,
    0b1000101010001010,
    0b1000001001001000,
    0b1000000000100000,
    0b1010000110100000,
    0b1000100000101000,
    0b1000100010010010,
    0b0010001000100010,
    0b0010010001000110,
    0b0010001000110010,
    0b1111101010111010,
    0b0000111100001100,
    0b1010110010101100,
    0b0011001100110010,
    0b0111011101110111,
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=Path(__file__).with_name("manifest.json"))
    parser.add_argument("--output", type=Path, default=Path(__file__).parents[1] / "code/src/apps/KastleChan/SAMPLES.bin")
    parser.add_argument("--generated", type=Path, default=Path(__file__).with_name("generated-v03-clear"))
    parser.add_argument("--force", action="store_true", help="Regenerate cached WAV clips")
    return parser.parse_args()


def flatten_bank(bank: dict) -> list[str]:
    roles = bank["roles"]
    if len(roles) != 4 or any(len(role) != 6 for role in roles):
        raise ValueError(f"{bank['id']}: expected four roles with six choices each")
    expressive = bank["expressive"]
    if len(expressive) != 8:
        raise ValueError(f"{bank['id']}: expected eight expressive clips")
    return [text for role in roles for text in role] + expressive


def synthesize_clip(
    tts: KokoroTTS,
    text: str,
    output: Path,
    voice: str,
    language: str,
    speed: float,
    sample_rate: int,
) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="kastle-chan-tts-") as temp_dir:
        source = Path(temp_dir) / "source.wav"
        tts.save(text, source, voice=voice, language=language, speed=speed, sample_rate=24000)

        # Raising the declared source rate gives the original voice a compact,
        # high virtual-idol register. The slower TTS speed keeps words readable.
        filters = (
            "silenceremove=start_periods=1:start_duration=0:start_threshold=-48dB:"
            "stop_periods=-1:stop_duration=0.04:stop_threshold=-48dB,"
            "highpass=f=90,lowpass=f=9000,"
            "acompressor=threshold=-18dB:ratio=2.5:attack=5:release=80,"
            "rubberband=pitch=1.16:formant=shifted:pitchq=quality,"
            "rubberband=pitch=0.862069:formant=preserved:pitchq=quality,"
            f"aresample={sample_rate},alimiter=limit=0.92,apad=pad_dur=0.05"
        )
        subprocess.run(
            [
                "ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
                "-i", str(source), "-af", filters,
                "-ar", str(sample_rate), "-ac", "1", "-c:a", "pcm_s16le", str(output),
            ],
            check=True,
        )


def read_pcm(path: Path, expected_rate: int) -> bytes:
    with wave.open(str(path), "rb") as wav:
        if wav.getframerate() != expected_rate or wav.getnchannels() != 1 or wav.getsampwidth() != 2:
            raise ValueError(f"unexpected WAV format: {path}")
        pcm = wav.readframes(wav.getnframes())
    if len(pcm) % 4:
        pcm += b"\0" * (4 - len(pcm) % 4)
    return pcm


def fixed_ascii(value: str, size: int) -> bytes:
    encoded = value.encode("ascii")
    if len(encoded) > size:
        raise ValueError(f"'{value}' exceeds {size} ASCII bytes")
    return encoded.ljust(size, b"\0")


def pack_bank(manifest: dict, wav_paths: list[list[Path]]) -> bytes:
    sample_rate = int(manifest["sample_rate"])
    banks = manifest["banks"]
    sample_count = 32

    body = bytearray()
    for scale in SCALES:
        body += struct.pack("<I", scale)
    for rhythm in RHYTHMS:
        body += struct.pack("<I", rhythm)

    for bank, paths in zip(banks, wav_paths, strict=True):
        name = fixed_ascii(bank["name"], 8)
        red, green, blue = bank["color"]
        body += struct.pack("<8sBBBB", name, red, green, blue, 0)
        for index, path in enumerate(paths):
            pcm = read_pcm(path, sample_rate)
            sample_name = fixed_ascii(f"{bank['id'][:3]}{index:02d}", 8)
            body += struct.pack("<IB8s3B", len(pcm), 1, sample_name, 0, 0, 0)
            body += pcm

    header_size = 20
    file_size = header_size + len(body) + 4
    header = struct.pack(
        "<4sII8B",
        b"k2kc",
        file_size,
        sample_rate,
        16,
        len(banks),
        sample_count,
        len(SCALES),
        len(RHYTHMS),
        16,
        0,
        0,
    )
    return header + body + b"ahoj"


def main() -> None:
    args = parse_args()
    manifest = json.loads(args.manifest.read_text())
    banks = manifest["banks"]
    if len(banks) != 6:
        raise ValueError("Kastle-chan requires exactly six personality banks")

    wav_paths: list[list[Path]] = []
    with KokoroTTS.from_pretrained() as tts:
        available = set(tts.list_voices())
        voice = manifest["voice"]
        if voice not in available:
            raise ValueError(f"voice '{voice}' unavailable; choices include {sorted(available)}")

        for bank in banks:
            texts = flatten_bank(bank)
            bank_paths: list[Path] = []
            for index, text in enumerate(texts):
                output = args.generated / bank["id"] / f"{index:02d}.wav"
                bank_paths.append(output)
                if args.force or not output.exists():
                    print(f"[{bank['id']} {index + 1:02d}/32] {text}", flush=True)
                    synthesize_clip(
                        tts,
                        text,
                        output,
                        manifest["voice"],
                        manifest["language"],
                        float(manifest["speed"]),
                        int(manifest["sample_rate"]),
                    )
            wav_paths.append(bank_paths)

    packed = pack_bank(manifest, wav_paths)
    if len(packed) > 7_500_000:
        raise ValueError(f"voice bank is too large: {len(packed):,} bytes")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(packed)
    print(f"Wrote {args.output} ({len(packed):,} bytes)")


if __name__ == "__main__":
    main()
