// format_helpers.h - разбор и форматирование чисел в текстовых полях формы.
// Пустое поле трактуется как "нет значения" (используется для необязательных
// настроек: конец фрагмента, транспонирование).
#pragma once
#include <optional>
#include <string>

namespace song2notes {

double parse_double(const std::wstring& text, double fallback);
int parse_int(const std::wstring& text, int fallback);
std::optional<double> parse_optional_double(const std::wstring& text);
std::optional<int> parse_optional_int(const std::wstring& text);

std::wstring format_double(double value, int decimals = 2);
std::wstring format_int(int value);
std::wstring format_optional_double(const std::optional<double>& value, int decimals = 2);
std::wstring format_optional_int(const std::optional<int>& value);

}  // namespace song2notes
