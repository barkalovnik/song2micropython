"""Загрузчик аудио (mp3/wav/ogg/flac): определение высоты тона одного голоса.

Паттерны: [Strategy] — роль «ConcreteStrategy».
"""
from __future__ import annotations

from collections import Counter
from pathlib import Path
from typing import ClassVar

from ..domain import ConversionSettings, NoteSegment, ProgressCallback
from ..errors import ConversionError, MissingDependencyError
from .base import SegmentLoader

_SAMPLE_RATE = 22050
_HOP = 256


class AudioLoader(SegmentLoader):
    """[Pattern: Strategy | Role: ConcreteStrategy] pYIN + детектор атак (librosa)."""

    extensions: ClassVar[frozenset[str]] = frozenset({".mp3", ".wav", ".ogg", ".flac"})

    def load(
        self, path: Path, settings: ConversionSettings, progress: ProgressCallback
    ) -> list[NoteSegment]:
        try:
            import librosa
            import numpy as np
        except ImportError as exc:
            raise MissingDependencyError(
                "Для аудио нужны библиотеки: pip install librosa numpy (и ffmpeg для mp3)"
            ) from exc

        duration = None if settings.end is None else settings.end - settings.start
        progress("Загрузка аудио...")
        try:
            y, sr = librosa.load(
                str(path), sr=_SAMPLE_RATE, mono=True, offset=settings.start, duration=duration
            )
        except Exception as exc:
            raise ConversionError(f"Не удалось прочитать аудио: {exc}") from exc

        progress("Определение высоты тона (может занять минуту)...")
        f0, voiced, _ = librosa.pyin(
            y, fmin=librosa.note_to_hz("C2"), fmax=librosa.note_to_hz("C7"),
            sr=sr, frame_length=2048, hop_length=_HOP,
        )
        mask = voiced & ~np.isnan(f0)
        pitches: list[int] = np.where(
            mask, np.round(librosa.hz_to_midi(np.nan_to_num(f0, nan=440.0))), -1
        ).astype(int).tolist()
        if not pitches:
            return []

        progress("Разбор нот...")
        smoothed = self._majority_filter(pitches, radius=2)
        onsets = {int(f) for f in librosa.onset.onset_detect(y=y, sr=sr, hop_length=_HOP)}
        return self._to_segments(smoothed, onsets, offset=settings.start, frame_s=_HOP / sr)

    @staticmethod
    def _majority_filter(values: list[int], radius: int) -> list[int]:
        """Сглаживание: в каждом кадре — самое частое значение в окне."""
        out: list[int] = []
        for i in range(len(values)):
            window = values[max(0, i - radius): i + radius + 1]
            out.append(Counter(window).most_common(1)[0][0])
        return out

    @staticmethod
    def _to_segments(
        pitches: list[int], onsets: set[int], offset: float, frame_s: float
    ) -> list[NoteSegment]:
        """Серии одинаковых кадров -> ноты; серия рвётся на атаках."""
        segments: list[NoteSegment] = []
        run_start, run_pitch = 0, pitches[0]
        n = len(pitches)
        for i in range(1, n + 1):
            boundary = i == n or pitches[i] != run_pitch or (i in onsets and run_pitch >= 0)
            if not boundary:
                continue
            if run_pitch >= 0:
                segments.append(
                    NoteSegment(offset + run_start * frame_s, offset + i * frame_s, run_pitch)
                )
            if i < n:
                run_start, run_pitch = i, pitches[i]
        return segments
