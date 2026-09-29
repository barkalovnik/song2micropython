#include "format_helpers.h"

#include <cwctype>
#include <sstream>

namespace song2notes {

namespace {
bool is_blank(const std::wstring& text) {
    for (wchar_t c : text) {
        if (!std::iswspace(c)) return false;
    }
    return true;
}
}  // namespace

double parse_double(const std::wstring& text, double fallback) {
    if (is_blank(text)) return fallback;
    try {
        size_t pos = 0;
        double v = std::stod(text, &pos);
        return v;
    } catch (...) {
        return fallback;
    }
}

int parse_int(const std::wstring& text, int fallback) {
    if (is_blank(text)) return fallback;
    try {
        size_t pos = 0;
        int v = std::stoi(text, &pos);
        return v;
    } catch (...) {
        return fallback;
    }
}

std::optional<double> parse_optional_double(const std::wstring& text) {
    if (is_blank(text)) return std::nullopt;
    try {
        return std::stod(text);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<int> parse_optional_int(const std::wstring& text) {
    if (is_blank(text)) return std::nullopt;
    try {
        return std::stoi(text);
    } catch (...) {
        return std::nullopt;
    }
}

std::wstring format_double(double value, int decimals) {
    std::wstringstream ss;
    ss.precision(decimals);
    ss << std::fixed << value;
    return ss.str();
}

std::wstring format_int(int value) { return std::to_wstring(value); }

std::wstring format_optional_double(const std::optional<double>& value, int decimals) {
    return value.has_value() ? format_double(*value, decimals) : L"";
}

std::wstring format_optional_int(const std::optional<int>& value) {
    return value.has_value() ? format_int(*value) : L"";
}

}  // namespace song2notes
