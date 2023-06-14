#include "NanoVDBLoader.h"
#include "Logger.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#define NANOVDB_USE_ZIP 1
#include <nanovdb/util/IO.h>
#include <fstream>
#include <sstream>


namespace IG {
// Only used in the building process

bool NanoVDBLoader::prepare(const std::filesystem::path& in_nvdb, const std::string gridName, const std::filesystem::path& out_data)
{
    if (!nanovdb::io::hasGrid(in_nvdb.u8string(), gridName)) {
        IG_LOG(L_ERROR) << "No grid found for grid name " << gridName << "." << std::endl;
        return false;
    }

    auto handle   = nanovdb::io::readGrid(in_nvdb.u8string(), gridName);
    uint8_t* data = handle.data(); // Returns a non-const pointer to the data
    uint64_t size = handle.size(); // Returns the size in bytes of the raw memory buffer

    auto dims   = handle.grid<float>()->indexBBox().dim();
    IG_LOG(L_DEBUG) << "Loading grid with dimensions (" << dims[0] << ", " << dims[1] << ", " << dims[2] << ")" << std::endl;

    const char* grid_type = toStr(handle.gridType());
    const char* grid_type_float = "float";
    const char* grid_type_end   = "End";
    if (strcmp(grid_type, grid_type_end) == 0) {
        IG_LOG(L_WARNING) << "No Grid found for grid name " << gridName << "." << std::endl;
        return false;
    } else if (strcmp(grid_type, grid_type_float) != 0) {
        // only float Grid Types (density) are currently supported in Ignis
        IG_LOG(L_ERROR) << "Only float Grid Types (density or temperature) are currently supported in Ignis (found " << grid_type << " for grid " << gridName << ")" << std::endl;
        return false;
    }

    if (handle.gridMetaData()->isUnknown()) {
        IG_LOG(L_WARNING) << "Only fog volumes are currently supported in Ignis, but VDB file has unknown gridclass. It will be interpreted as a Fog volume but may not produce the desired results." << std::endl;
    }
    else if (!handle.gridMetaData()->isFogVolume()) {
        // only fog volumes are currently supported in Ignis
        IG_LOG(L_ERROR) << "Only fog volumes are currently supported in Ignis ( found " << toStr(handle.gridMetaData()->gridClass()) << ")" << std::endl;
        return false;
    }
        
    std::ofstream stream(out_data.u8string(), std::ios::binary | std::ios::trunc);
    stream.write((char*)&data[0], size);
    stream.close();
    return true;
}

bool NanoVDBLoader::prepare_uniform_grid(const std::filesystem::path& in_nvdb, const std::filesystem::path& out_data, const std::string gridNameDensity, const std::string gridNameTemperature)
{
    const bool hasTemp = gridNameTemperature != "none";
    if (!hasTemp)
        IG_LOG(L_DEBUG) << "No Temperature grid (" << gridNameTemperature << ")" << std::endl;
        
    auto handleDensity = nanovdb::io::readGrid(in_nvdb.u8string(), gridNameDensity);
    auto handleTemperature = nanovdb::io::readGrid(in_nvdb.u8string(), gridNameTemperature);

    const char* grid_type_density = toStr(handleDensity.gridType());
    const char* grid_type_temperature = toStr(handleTemperature.gridType());
    const char* grid_type_float = "float";
    const char* grid_type_end   = "End";
    if (strcmp(grid_type_density, grid_type_end) == 0) {
        IG_LOG(L_WARNING) << "No Grid found for grid name " << gridNameDensity << "." << std::endl;
        return false;
    } else if (hasTemp && strcmp(grid_type_temperature, grid_type_end) == 0) {
        IG_LOG(L_WARNING) << "No Grid found for grid name " << gridNameDensity << "." << std::endl;
        return false;
    } else if (strcmp(grid_type_density, grid_type_float) != 0) {
        // only float Grid Types (density) are currently supported in Ignis
        IG_LOG(L_ERROR) << "Only float Grid Types (density or temperature) are currently supported in Ignis (found " << grid_type_density << " for grid " << gridNameDensity << ")" << std::endl;
        return false;
    } else if (hasTemp && strcmp(grid_type_temperature, grid_type_float) != 0) {
        // only float Grid Types (temperature) are currently supported in Ignis
        IG_LOG(L_ERROR) << "Only float Grid Types (density or temperature) are currently supported in Ignis (found " << grid_type_temperature << " for grid " << gridNameTemperature << ")" << std::endl;
        return false;
    }

    if (handleDensity.gridMetaData()->isUnknown()) {
        IG_LOG(L_WARNING) << "Only fog volumes are currently supported in Ignis, but VDB file has unknown gridclass. It will be interpreted as a Fog volume but may not produce the desired results." << std::endl;
    } else if (!handleDensity.gridMetaData()->isFogVolume()) {
        // only fog volumes are currently supported in Ignis
        IG_LOG(L_ERROR) << "Only fog volumes are currently supported in Ignis ( found " << toStr(handleDensity.gridMetaData()->gridClass()) << ")" << std::endl;
        return false;
    }

    if (hasTemp && handleTemperature.gridMetaData()->isUnknown()) {
        IG_LOG(L_WARNING) << "Only fog volumes are currently supported in Ignis, but VDB file has unknown gridclass. It will be interpreted as a Fog volume but may not produce the desired results." << std::endl;
    } else if (hasTemp && !handleTemperature.gridMetaData()->isFogVolume()) {
        // only fog volumes are currently supported in Ignis
        IG_LOG(L_ERROR) << "Only fog volumes are currently supported in Ignis ( found " << toStr(handleTemperature.gridMetaData()->gridClass()) << ")" << std::endl;
        return false;
    }
    
    std::ofstream stream(out_data.u8string(), std::ios::binary | std::ios::trunc);
    auto* gridDensity = handleDensity.grid<float>();
    auto* gridTemperature = hasTemp ? handleTemperature.grid<float>() : nullptr;

    auto bbox   = gridDensity->indexBBox();
    auto dims   = bbox.dim();
    auto min    = bbox.min();
    auto max    = bbox.max();
    auto width  = static_cast<uint32>(dims[0]);
    auto height = static_cast<uint32>(dims[1]);
    auto depth  = static_cast<uint32>(dims[2]);

    auto zero = static_cast<uint32>(0);
    auto one  = static_cast<uint32>(1);
    auto zero_f32 = static_cast<float>(0);

    stream.write(reinterpret_cast<char*>(&width),  sizeof(width));
    stream.write(reinterpret_cast<char*>(&height), sizeof(height));
    stream.write(reinterpret_cast<char*>(&depth),  sizeof(depth));

    IG_ASSERT(max[0] + 1 - min[0] == dims[0], "Expected correct dimensions");
    IG_ASSERT(max[1] + 1 - min[1] == dims[1], "Expected correct dimensions");
    IG_ASSERT(max[2] + 1 - min[2] == dims[2], "Expected correct dimensions");

    if (hasTemp)
        stream.write(reinterpret_cast<char*>(&one), sizeof(one));
    else
        stream.write(reinterpret_cast<char*>(&zero), sizeof(zero));

    for (int32_t z = min[2]; z <= max[2]; z++) {
        for (int32_t y = min[1]; y <= max[1]; y++) {
            for (int32_t x = min[0]; x <= max[0]; x++) {
                auto valueD = static_cast<float>(gridDensity->tree().getValue(nanovdb::Coord(x, y, z)));
                stream.write(reinterpret_cast<char*>(&valueD), sizeof(float));
                if (hasTemp) {
                    auto valueT = static_cast<float>(gridTemperature->tree().getValue(nanovdb::Coord(x, y, z)));
                    stream.write(reinterpret_cast<char*>(&valueT), sizeof(float));
                } else {
                    stream.write(reinterpret_cast<char*>(&zero_f32), sizeof(float));
                }
            }
        }
    }

    stream.close();
    return true;
}

} // Namespace IG
