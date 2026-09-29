"""Командная строка. Использует тот же Facade (ConversionService), что и GUI."""
from __future__ import annotations

import argparse
import sys
from collections.abc import Sequence
from pathlib import Path

from .domain import ConversionSettings
from .errors import ConversionError
from .exporters import all_exporters, get_exporter
from .service import ConversionService


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(prog="song2notes", description="MIDI/MP3 -> мелодия для пищалки")
    ap.add_argument("input", type=Path, help=".mid/.midi/.mp3/.wav/.ogg/.flac")
    ap.add_argument("-o", "--output", type=Path, help="выходной файл")
    ap.add_argument("--format", default="micropython", choices=[e.key for e in all_exporters()])
    ap.add_argument("--pin", type=int, default=26)
    ap.add_argument("--track", type=int)
    ap.add_argument("--list", action="store_true", help="показать дорожки MIDI и выйти")
    ap.add_argument("--start", type=float, default=0.0)
    ap.add_argument("--end", type=float)
    ap.add_argument("--speed", type=float, default=1.0)
    ap.add_argument("--transpose", type=int)
    ap.add_argument("--min-note-ms", type=int, default=60)
    ap.add_argument("--max-notes", type=int, default=1500)
    return ap


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    service = ConversionService()
    try:
        if args.list:
            tracks = service.inspect(args.input)
            if not tracks:
                print("Выбор дорожек недоступен для этого файла.")
            for t in tracks:
                print(f"{t.index}: {t.name or '(без имени)'}  нот: {t.note_count}")
            return 0

        settings = ConversionSettings(
            start=args.start, end=args.end, track=args.track, speed=args.speed,
            transpose=args.transpose, min_note_ms=args.min_note_ms,
            max_notes=args.max_notes, pin=args.pin, exporter_key=args.format,
        )
        result = service.convert(args.input, settings, progress=print)
        text = service.render(result.events, settings, args.input.name)
    except ConversionError as exc:
        print(f"Ошибка: {exc}", file=sys.stderr)
        return 1

    suffix = get_exporter(args.format).file_suffix
    out = args.output or args.input.with_name(f"{args.input.stem}_melody{suffix}")
    out.write_text(text, encoding="utf-8")
    note = " (обрезано по --max-notes)" if result.truncated else ""
    print(f"Готово: {out} — {len(result.events)} событий, ~{result.total_ms / 1000:.1f} с{note}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
