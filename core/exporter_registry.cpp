#include "exporter_registry.h"

#include "csv_exporter.h"
#include "errors.h"
#include "micropython_exporter.h"

namespace song2notes {

namespace {
std::vector<std::unique_ptr<Exporter>>& registry() {
    static std::vector<std::unique_ptr<Exporter>> instances = [] {
        std::vector<std::unique_ptr<Exporter>> v;
        v.push_back(std::make_unique<MicroPythonExporter>());
        v.push_back(std::make_unique<CsvExporter>());
        return v;
    }();
    return instances;
}
}  // namespace

const Exporter& get_exporter(const std::string& key) {
    for (const auto& e : registry()) {
        if (e->key() == key) return *e;
    }
    throw ConversionError("Неизвестный формат экспорта: " + key);
}

std::vector<const Exporter*> all_exporters() {
    std::vector<const Exporter*> out;
    for (const auto& e : registry()) out.push_back(e.get());
    return out;
}

}  // namespace song2notes
