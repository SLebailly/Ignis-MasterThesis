#pragma once

#include "IG_Config.h"

namespace IG {
struct NanoVDBSpecification {
    //TODO: implement thisi if and as required
};

class NanoVDBLoader {
public:
    static bool prepare(const std::filesystem::path& in_nvdb, const std::string gridName, const std::filesystem::path& out_data);
    static bool prepare_naive_grid(const std::filesystem::path& in_nvdb, const std::filesystem::path& out_data);
};
} // namespace IG