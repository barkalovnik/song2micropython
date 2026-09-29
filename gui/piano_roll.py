"""Виджет-«пианоролл» для предпросмотра мелодии."""
from __future__ import annotations

from collections.abc import Sequence

from PyQt6.QtCore import QRectF, Qt
from PyQt6.QtGui import QPainter, QPaintEvent
from PyQt6.QtWidgets import QSizePolicy, QWidget

from ..domain import NoteEvent


class PianoRollWidget(QWidget):
    """Простая визуализация: время по горизонтали, высота ноты по вертикали."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._events: tuple[NoteEvent, ...] = ()
        self._total_ms = 0
        self.setMinimumHeight(140)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Preferred)

    def set_events(self, events: Sequence[NoteEvent]) -> None:
        self._events = tuple(events)
        self._total_ms = sum(e.duration_ms for e in self._events)
        self.update()

    def paintEvent(self, event: QPaintEvent | None) -> None:
        painter = QPainter(self)
        painter.fillRect(self.rect(), self.palette().base())
        pitches = [e.pitch for e in self._events if e.pitch is not None]
        if not pitches or self._total_ms <= 0:
            painter.setPen(self.palette().placeholderText().color())
            painter.drawText(self.rect(), Qt.AlignmentFlag.AlignCenter, "Здесь появится мелодия")
            painter.end()
            return

        low, high = min(pitches), max(pitches)
        rows = high - low + 1
        width, height = float(self.width()), float(self.height())
        row_h = height / rows
        brush = self.palette().highlight()
        elapsed = 0
        for e in self._events:
            if e.pitch is not None:
                x = elapsed / self._total_ms * width
                w = max(1.0, e.duration_ms / self._total_ms * width - 0.5)
                y = height - (e.pitch - low + 1) * row_h
                painter.fillRect(QRectF(x, y, w, max(1.0, row_h - 1.0)), brush)
            elapsed += e.duration_ms
        painter.end()
