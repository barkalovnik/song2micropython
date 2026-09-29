"""Конвейер обработки сегментов.

Паттерны:
  [Pipeline / Pipes and Filters] — Pipeline последовательно применяет шаги;
  [Strategy] — каждый ProcessingStep взаимозаменяем;
  [Factory Function] — build_pipeline() собирает конвейер из настроек.
"""
from __future__ import annotations

from abc import ABC, abstractmethod
from collections.abc import Iterable, Sequence

from .domain import ConversionSettings, NoteSegment
from .notes import PITCH_HIGH, PITCH_LOW


class ProcessingStep(ABC):
    """[Pattern: Pipeline | Role: Filter] Один шаг обработки: сегменты -> сегменты."""

    @abstractmethod
    def process(self, segments: Sequence[NoteSegment]) -> list[NoteSegment]: ...


class CropStep(ProcessingStep):
    """[Pattern: Pipeline | Role: ConcreteFilter] Вырезает фрагмент [start, end) и сдвигает к нулю."""

    def __init__(self, start: float, end: float | None) -> None:
        self._start = start
        self._end = end

    def process(self, segments: Sequence[NoteSegment]) -> list[NoteSegment]:
        out: list[NoteSegment] = []
        for s in segments:
            if s.end <= self._start or (self._end is not None and s.start >= self._end):
                continue
            new_start = max(s.start, self._start)
            new_end = s.end if self._end is None else min(s.end, self._end)
            out.append(NoteSegment(new_start - self._start, new_end - self._start, s.pitch))
        return out


class MinDurationStep(ProcessingStep):
    """[Pattern: Pipeline | Role: ConcreteFilter] Отбрасывает слишком короткие ноты (шум)."""

    def __init__(self, min_ms: int) -> None:
        self._min_s = min_ms / 1000

    def process(self, segments: Sequence[NoteSegment]) -> list[NoteSegment]:
        return [s for s in segments if s.duration >= self._min_s]


class FitRangeStep(ProcessingStep):
    """[Pattern: Pipeline | Role: ConcreteFilter] Транспонирует и «сворачивает» ноты в диапазон пищалки."""

    def __init__(self, transpose: int | None) -> None:
        self._transpose = transpose

    def process(self, segments: Sequence[NoteSegment]) -> list[NoteSegment]:
        if not segments:
            return []
        shift = self._transpose if self._transpose is not None else self._best_shift(segments)
        return [NoteSegment(s.start, s.end, self._fold(s.pitch + shift)) for s in segments]

    @staticmethod
    def _fold(pitch: int) -> int:
        while pitch < PITCH_LOW:
            pitch += 12
        while pitch > PITCH_HIGH:
            pitch -= 12
        return pitch

    @staticmethod
    def _best_shift(segments: Sequence[NoteSegment]) -> int:
        """Сдвиг по октавам, при котором больше всего нот попадает в диапазон."""
        return max(
            range(-48, 49, 12),
            key=lambda k: (sum(PITCH_LOW <= s.pitch + k <= PITCH_HIGH for s in segments), -abs(k)),
        )


class Pipeline:
    """[Pattern: Pipeline | Role: Pipeline] Последовательно применяет шаги."""

    def __init__(self, steps: Iterable[ProcessingStep]) -> None:
        self._steps = list(steps)

    def run(self, segments: Sequence[NoteSegment]) -> list[NoteSegment]:
        current = list(segments)
        for step in self._steps:
            current = step.process(current)
        return current


def build_pipeline(settings: ConversionSettings) -> Pipeline:
    """[Pattern: Factory Function] Собирает конвейер под заданные настройки."""
    return Pipeline([
        CropStep(settings.start, settings.end),
        MinDurationStep(settings.min_note_ms),
        FitRangeStep(settings.transpose),
    ])
