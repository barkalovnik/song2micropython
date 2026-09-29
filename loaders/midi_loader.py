"""Загрузчик MIDI.

Паттерны: [Strategy] — роль «ConcreteStrategy».
Внутри — алгоритм «skyline»: из аккордов берётся самая высокая нота.
"""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any, ClassVar

from ..domain import ConversionSettings, NoteSegment, ProgressCallback, TrackInfo
from ..errors import ConversionError, MissingDependencyError
from .base import SegmentLoader


@dataclass(slots=True)
class _Span:
    start: float
    end: float
    pitch: int
    note_id: int


def skyline(notes: list[tuple[float, float, int]]) -> list[NoteSegment]:
    """Одноголосие: в каждый момент времени звучит самая высокая из активных нот."""
    ordered = sorted(notes)
    bounds = sorted({t for s, e, _ in ordered for t in (s, e)})
    spans: list[_Span] = []
    active: list[tuple[int, tuple[float, float, int]]] = []
    idx = 0
    for t0, t1 in zip(bounds, bounds[1:]):
        while idx < len(ordered) and ordered[idx][0] <= t0 + 1e-9:
            active.append((idx, ordered[idx]))
            idx += 1
        active = [a for a in active if a[1][1] > t0 + 1e-9]
        if not active:
            continue
        note_id, (_, _, pitch) = max(active, key=lambda a: a[1][2])
        if spans and spans[-1].note_id == note_id and abs(spans[-1].end - t0) < 1e-9:
            spans[-1].end = t1  # продолжение той же ноты
        else:
            spans.append(_Span(t0, t1, pitch, note_id))
    return [NoteSegment(s.start, s.end, s.pitch) for s in spans]


class MidiLoader(SegmentLoader):
    """[Pattern: Strategy | Role: ConcreteStrategy] Чтение .mid/.midi через pretty_midi."""

    extensions: ClassVar[frozenset[str]] = frozenset({".mid", ".midi"})

    def list_tracks(self, path: Path) -> list[TrackInfo]:
        return [self._info(i, ins) for i, ins in self._melodic(self._open(path))]

    def load(
        self, path: Path, settings: ConversionSettings, progress: ProgressCallback
    ) -> list[NoteSegment]:
        progress("Чтение MIDI...")
        candidates = self._melodic(self._open(path))
        if not candidates:
            raise ConversionError("В MIDI нет мелодических дорожек.")
        by_index = dict(candidates)
        index = settings.track if settings.track is not None else self._auto_track(candidates)
        if index not in by_index:
            raise ConversionError(f"В файле нет дорожки с номером {index}.")
        progress(f"Выделение мелодии из дорожки {index}...")
        instrument = by_index[index]
        return skyline([(n.start, n.end, n.pitch) for n in instrument.notes])

    # --- внутренние помощники ---------------------------------------------
    @staticmethod
    def _open(path: Path) -> Any:
        try:
            import pretty_midi
        except ImportError as exc:
            raise MissingDependencyError("Для MIDI нужна библиотека: pip install pretty_midi") from exc
        try:
            return pretty_midi.PrettyMIDI(str(path))
        except Exception as exc:  # pretty_midi бросает разные типы на битых файлах
            raise ConversionError(f"Не удалось прочитать MIDI: {exc}") from exc

    @staticmethod
    def _melodic(pm: Any) -> list[tuple[int, Any]]:
        return [(i, ins) for i, ins in enumerate(pm.instruments) if not ins.is_drum and ins.notes]

    @staticmethod
    def _mean_pitch(instrument: Any) -> int:
        return int(sum(n.pitch for n in instrument.notes) / len(instrument.notes))

    def _info(self, index: int, instrument: Any) -> TrackInfo:
        return TrackInfo(index, instrument.name, len(instrument.notes), self._mean_pitch(instrument))

    def _auto_track(self, candidates: list[tuple[int, Any]]) -> int:
        """Больше всего нот среди небасовых дорожек."""
        non_bass = [c for c in candidates if self._mean_pitch(c[1]) >= 55] or candidates
        return max(non_bass, key=lambda c: len(c[1].notes))[0]
