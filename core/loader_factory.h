// loader_factory.h - выбор загрузчика по расширению файла.
// [Pattern: Factory | Role: Creator] + [Pattern: Registry]
#pragma once
#include <memory>
#include <vector>

#include "segment_loader.h"

namespace song2notes {

class LoaderFactory {
public:
    LoaderFactory();
    const SegmentLoader& create(const std::wstring& path) const;
    std::vector<std::wstring> supported_extensions() const { return {L".mid", L".midi"}; }

private:
    std::unique_ptr<SegmentLoader> midi_loader_;
};

}  // namespace song2notes
