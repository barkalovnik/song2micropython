#include "loader_factory.h"

#include <algorithm>
#include <cwctype>

#include "errors.h"
#include "midi_loader.h"
#include "text_convert.h"

namespace song2notes {

namespace {
std::wstring lower_suffix(const std::wstring& path) {
    size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos) return L"";
    std::wstring suffix = path.substr(dot);
    std::transform(suffix.begin(), suffix.end(), suffix.begin(), [](wchar_t c) { return std::towlower(c); });
    return suffix;
}
}  // namespace

LoaderFactory::LoaderFactory() : midi_loader_(std::make_unique<MidiLoader>()) {}

const SegmentLoader& LoaderFactory::create(const std::wstring& path) const {
    std::wstring suffix = lower_suffix(path);
    if (suffix == L".mid" || suffix == L".midi") return *midi_loader_;
    throw UnsupportedFormatError("Формат «" + narrow_utf8(suffix) +
                                  "» не поддерживается. Поддерживаются только файлы .mid и .midi.");
}

}  // namespace song2notes
