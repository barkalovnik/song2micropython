// exporter_registry.h - реестр экспортёров, доступ к Strategy по ключу.
// [Pattern: Registry]
#pragma once
#include <memory>
#include <vector>

#include "exporter.h"

namespace song2notes {

const Exporter& get_exporter(const std::string& key);
std::vector<const Exporter*> all_exporters();

}  // namespace song2notes
