// text_convert.h - преобразования между UTF-8 (std::string, используется для
// текста генерируемого файла) и UTF-16 (std::wstring, используется везде,
// где участвует WinAPI: заголовок окна, диалоги, MessageBox).
#pragma once
#include <string>

namespace song2notes {

std::wstring widen_utf8(const std::string& utf8);
std::string narrow_utf8(const std::wstring& wide);
std::wstring filename_only(const std::wstring& path);

}  // namespace song2notes
