"""Реестр экспортёров.

Паттерн: [Registry] — доступ к Strategy-экспортёрам по ключу.
"""
from __future__ import annotations

from ..errors import ConversionError
from .base import Exporter
from .csv_exporter import CsvExporter
from .micropython import MicroPythonExporter

_REGISTRY: dict[str, Exporter] = {e.key: e for e in (MicroPythonExporter(), CsvExporter())}


def get_exporter(key: str) -> Exporter:
    try:
        return _REGISTRY[key]
    except KeyError:
        raise ConversionError(f"Неизвестный формат экспорта: {key}") from None


def all_exporters() -> list[Exporter]:
    return list(_REGISTRY.values())


__all__ = ["Exporter", "get_exporter", "all_exporters"]
