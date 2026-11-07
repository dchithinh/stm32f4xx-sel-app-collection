#!/usr/bin/env python3
"""Normalize leading indentation to 4 spaces per level (spaces only, no tabs)."""
from __future__ import annotations

import argparse
import re
from pathlib import Path

SKIP_PARTS = {
    "lvgl",
    "helix",
    ".git",
    "build",
    ".pio",
}


def detect_unit(lines: list[str]) -> int:
    widths: list[int] = []
    for line in lines:
        if not line.strip():
            continue
        m = re.match(r"^ +", line)
        if not m:
            continue
        n = len(m.group(0))
        if n > 0:
            widths.append(n)
    if not widths:
        return 4
    for unit in (2, 4, 8):
        if all(w % unit == 0 for w in widths):
            return unit
    return 4


def reindent_text(text: str) -> str:
    lines = text.splitlines(keepends=True)
    unit = detect_unit([l.rstrip("\r\n") for l in lines])
    out: list[str] = []
    for line in lines:
        nl = ""
        if line.endswith("\r\n"):
            nl = "\r\n"
            body = line[:-2]
        elif line.endswith("\n"):
            nl = "\n"
            body = line[:-1]
        else:
            body = line
        if not body.strip():
            out.append(nl if body == "" else body + nl)
            continue
        stripped = body.lstrip(" \t")
        lead = len(body) - len(stripped)
        if "\t" in body[:lead]:
            # expand tabs in leading whitespace only (tab stop 4)
            prefix = body[:lead].expandtabs(4)
            lead = len(prefix)
        levels = lead // unit if unit else 0
        if lead % unit != 0:
            levels = (lead + unit - 1) // unit
        out.append((" " * (levels * 4)) + stripped + nl)
    return "".join(out)


def should_skip(path: Path) -> bool:
    return any(p in SKIP_PARTS for p in path.parts)


def collect(root: Path, globs: list[str]) -> list[Path]:
    files: list[Path] = []
    for g in globs:
        for p in root.glob(g):
            if p.is_file() and not should_skip(p):
                files.append(p)
    return sorted(set(files))


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("root", type=Path, nargs="?", default=Path(__file__).resolve().parents[1])
    ap.add_argument("--write", action="store_true")
    args = ap.parse_args()
    root: Path = args.root

    patterns = [
        "lib/cli/**/*.c",
        "lib/cli/**/*.h",
        "lib/logger/*.c",
        "lib/logger/*.h",
        "lib/stm32f407_hat_bsp/**/*.c",
        "lib/stm32f407_hat_bsp/**/*.h",
        "lib/sdcard/**/*.c",
        "lib/sdcard/**/*.h",
        "lib/fatfs/diskio.c",
        "lib/fatfs/diskio.h",
        "lib/fatfs/ffconf.h",
        "lib/mbzm/mbzm_uart_io.c",
        "lib/mbzm/zmodem_fatfs.c",
        "lib/mbzm/include/zmodem_fatfs.h",
        "stm32f407_music_player/src/**/*.c",
        "stm32f407_music_player/include/**/*.h",
        "stm32f407_music_player/lib/audio/**/*.c",
        "stm32f407_music_player/lib/audio/**/*.h",
        "stm32f407_pomodoro/Core/**/*.c",
        "stm32f407_pomodoro/Core/**/*.h",
    ]

    files = collect(root, patterns)
    changed = 0
    for path in files:
        raw = path.read_text(encoding="utf-8", errors="replace")
        new = reindent_text(raw)
        if new != raw:
            changed += 1
            if args.write:
                path.write_text(new, encoding="utf-8", newline="\n")
            else:
                print(path)
    print(f"{'Updated' if args.write else 'Would update'} {changed} / {len(files)} files")


if __name__ == "__main__":
    main()
