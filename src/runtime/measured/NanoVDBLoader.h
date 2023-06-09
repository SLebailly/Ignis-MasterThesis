#pragma once

#include "IG_Config.h"

namespace IG {
struct NanoVDBSpecification {
    //TODO: implement this if and as required
};

class NanoVDBLoader {
public:
    static bool prepare(const std::filesystem::path& in_nvdb, const std::string gridName, const std::filesystem::path& out_data);
    static bool prepare_uniform_grid(const std::filesystem::path& in_nvdb, const std::filesystem::path& out_data, const std::string gridNameDensity, const std::string gridNameTemperature);
};
} // namespace IG