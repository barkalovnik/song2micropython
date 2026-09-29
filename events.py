"""Преобразование сегментов на временной шкале в последовательность событий."""
from __future__ import annotations

from collections.abc import Sequence

from .domain import NoteEvent, NoteSegment

_MIN_REST_S = 0.02  # щели короче 20 мс не считаем паузой, а растягиваем ноту


def segments_to_events(segments: Sequence[NoteSegment], speed: float = 1.0) -> list[NoteEvent]:
    """Превращает сегменты в ноты и паузы. speed=2 — вдвое быстрее."""
    events: list[NoteEvent] = []
    cursor = 0.0
    for seg in sorted(segments, key=lambda s: s.start):
        gap = seg.start - cursor
        if gap > _MIN_REST_S:
            events.append(NoteEvent(None, int(gap * 1000 / speed)))
            note_start = seg.start
        else:
            note_start = cursor
        if seg.end <= note_start:
            continue
        events.append(NoteEvent(seg.pitch, max(1, int((seg.end - note_start) * 1000 / speed))))
        cursor = seg.end
    return events
