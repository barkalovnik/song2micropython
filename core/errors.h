// errors.h - иерархия ошибок конвертации. Сообщения написаны так, чтобы их
// можно было показать пользователю как есть (в MessageBox).
#pragma once
#include <stdexcept>
#include <string>

namespace song2notes {

class ConversionError : public std::runtime_error {
public:
    explicit ConversionError(const std::string& msg) : std::runtime_error(msg) {}
};

class UnsupportedFormatError : public ConversionError {
public:
    explicit UnsupportedFormatError(const std::string& msg) : ConversionError(msg) {}
};

class EmptyMelodyError : public ConversionError {
public:
    explicit EmptyMelodyError(const std::string& msg) : ConversionError(msg) {}
};

}  // namespace song2notes
