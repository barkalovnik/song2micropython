"""Иерархия ошибок. Сообщения предназначены для показа пользователю как есть."""
from __future__ import annotations


class ConversionError(RuntimeError):
    """Базовая ошибка конвертации."""


class UnsupportedFormatError(ConversionError):
    """Для расширения файла нет подходящего загрузчика."""


class MissingDependencyError(ConversionError):
    """Не установлена нужная сторонняя библиотека."""


class EmptyMelodyError(ConversionError):
    """После обработки не осталось ни одной ноты."""
