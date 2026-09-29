// exporter.h - базовый экспортёр.
// [Pattern: Template Method | Role: AbstractClass] export_text() задаёт
// скелет (header -> body -> footer), подклассы переопределяют шаги.
// [Pattern: Strategy | Role: Strategy] экспортёры взаимозаменяемы.
#pragma once
#include <string>
#include <vector>

#include "domain.h"

namespace song2notes {

class Exporter {
public:
    virtual ~Exporter() = default;

    virtual std::string key() const = 0;
    virtual std::string label() const = 0;
    virtual std::string file_suffix() const = 0;

    // Шаблонный метод.
    std::string export_text(const std::vector<NoteEvent>& events, const ConversionSettings& settings,
                             const std::string& source_name) const {
        std::string text;
        text += header(events, settings, source_name);
        text += body(events, settings);
        text += footer(events, settings);
        return text;
    }

protected:
    virtual std::string header(const std::vector<NoteEvent>&, const ConversionSettings&,
                                const std::string&) const {
        return "";
    }
    virtual std::string body(const std::vector<NoteEvent>& events, const ConversionSettings& settings) const = 0;
    virtual std::string footer(const std::vector<NoteEvent>&, const ConversionSettings&) const { return ""; }
};

}  // namespace song2notes
