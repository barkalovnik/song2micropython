// io_utils.h - чтение/запись файлов через WinAPI напрямую (а не std::ifstream
// с широким путём), чтобы код одинаково собирался и под MSVC, и под MinGW:
// у MinGW нет стандартного конструктора std::ifstream(std::wstring).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace song2notes {

// Бросает ConversionError при любой ошибке доступа к файлу.
std::vector<uint8_t> read_file_bytes(const std::wstring& path);
void write_file_bytes(const std::wstring& path, const std::string& utf8_content);

}  // namespace song2notes
