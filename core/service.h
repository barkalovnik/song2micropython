// service.h - прикладной сервис.
// [Pattern: Facade | Role: Facade] Единая точка входа над загрузчиками
// (Strategy+Factory), конвейером (Pipeline) и экспортёрами (Template Method).
// И GUI-контроллер, и (при желании) консольная обвязка знают только этот класс.
#pragma once
#include <functional>
#include <string>
#include <vector>

#include "domain.h"
#include "loader_factory.h"

namespace song2notes {

class ConversionService {
public:
    ConversionService() = default;

    std::vector<std::wstring> supported_extensions() const { return loaders_.supported_extensions(); }

    std::vector<TrackInfo> inspect(const std::wstring& path) const;

    ConversionResult convert(const std::wstring& path, const ConversionSettings& settings,
                              const std::function<void(const std::wstring&)>& progress) const;

    std::string render(const std::vector<NoteEvent>& events, const ConversionSettings& settings,
                        const std::string& source_name) const;

private:
    LoaderFactory loaders_;
    static void validate(const ConversionSettings& settings);
};

}  // namespace song2notes
