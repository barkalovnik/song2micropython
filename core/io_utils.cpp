#include "io_utils.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "errors.h"

namespace song2notes {

std::vector<uint8_t> read_file_bytes(const std::wstring& path) {
    HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throw ConversionError("Не удалось открыть файл. Проверьте путь и права доступа.");
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(handle, &size) || size.QuadPart < 0) {
        CloseHandle(handle);
        throw ConversionError("Не удалось определить размер файла.");
    }
    std::vector<uint8_t> data(static_cast<size_t>(size.QuadPart));
    DWORD read_total = 0;
    if (!data.empty()) {
        if (!ReadFile(handle, data.data(), static_cast<DWORD>(data.size()), &read_total, nullptr) ||
            read_total != data.size()) {
            CloseHandle(handle);
            throw ConversionError("Ошибка чтения файла.");
        }
    }
    CloseHandle(handle);
    return data;
}

void write_file_bytes(const std::wstring& path, const std::string& utf8_content) {
    HANDLE handle = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throw ConversionError("Не удалось создать файл для сохранения.");
    }
    DWORD written = 0;
    BOOL ok = TRUE;
    if (!utf8_content.empty()) {
        ok = WriteFile(handle, utf8_content.data(), static_cast<DWORD>(utf8_content.size()), &written, nullptr);
    }
    CloseHandle(handle);
    if (!ok || written != utf8_content.size()) {
        throw ConversionError("Ошибка записи файла.");
    }
}

}  // namespace song2notes
