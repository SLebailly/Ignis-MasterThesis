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
    auto handle   = nanovdb::io::readGrid(in_nvdb.u8string(), gridName);
    uint8_t* data = handle.data(); // Returns a non-const pointer to the data
    uint64_t size = handle.size(); // Returns the size in bytes of the raw memory buffer

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


    
    auto* grid = handle.grid<float>();
    auto dims   = grid->indexBBox().dim();
    auto width  = static_cast<uint32>(dims[0]);
    auto height = static_cast<uint32>(dims[1]);
    auto depth  = static_cast<uint32>(dims[2]);

    //ASSERT
    uint32_t control_x = width / 2;
    uint32_t control_y = height / 2;
    uint32_t control_z = depth / 2;
    IG_LOG(L_DEBUG) << "Writing Volume with dimensions: " << width << ", " << height << ", " << depth << std::endl;
    IG_LOG(L_DEBUG) << "Control coordinate (" << control_x << ", " << control_y << ", " << control_z << ") with value: " << grid->tree().getValue(nanovdb::Coord(control_x, control_y, control_z)) << std::endl;
    IG_LOG(L_DEBUG) << "Control coordinate (" << control_x + 10 << ", " << control_y + 10 << ", " << control_z + 10 << ") with value: " << grid->tree().getValue(nanovdb::Coord(control_x + 10 , control_y + 10 , control_z + 10 )) << std::endl;
    IG_LOG(L_DEBUG) << "Control coordinate (" << control_x - 10 << ", " << control_y - 10 << ", " << control_z - 10 << ") with value: " << grid->tree().getValue(nanovdb::Coord(control_x - 10, control_y - 10, control_z - 10)) << std::endl;

    
    std::ofstream stream(out_data.u8string(), std::ios::binary | std::ios::trunc);
    stream.write((char*)&data[0], size);
    stream.close();
    return true;
}

//TODO: add PrincipledVolumeShader support
bool NanoVDBLoader::prepare_naive_grid(const std::filesystem::path& in_nvdb, const std::filesystem::path& out_data)
{
    auto handle   = nanovdb::io::readGrid(in_nvdb.u8string());
    //uint8_t* data = handle.data(); // Returns a non-const pointer to the data
    //uint64_t size = handle.size(); // Returns the size in bytes of the raw memory buffer

    if (!handle.gridMetaData()->isFogVolume()) {
        // only fog volumes are currently supported in Ignis
        IG_LOG(L_ERROR) << "Only fog volumes are currently supported in Ignis" << std::endl;
        return false;
    }

    const char* grid_type = toStr(handle.gridType());
    const char* grid_type_float = "float";
    int result = strcmp(grid_type, grid_type_float);
    if (result != 0) {
        // only float Grid Types (density) are currently supported in Ignis
        IG_LOG(L_ERROR) << "Only float Grid Types (density) are currently supported in Ignis (found " << grid_type << ")" << std::endl;
        return false;
    }
    
    std::ofstream stream(out_data.u8string(), std::ios::binary | std::ios::trunc);
    auto* grid = handle.grid<float>();

    auto dims   = grid->indexBBox().dim();
    auto width  = static_cast<uint32>(dims[0]);
    auto height = static_cast<uint32>(dims[1]);
    auto depth  = static_cast<uint32>(dims[2]);

    auto zero = static_cast<uint32>(0);
    auto zero_f32 = static_cast<float>(0);


    uint32_t control_x = width / 2;
    uint32_t control_y = height / 2;
    uint32_t control_z = depth / 2;
    IG_LOG(L_INFO) << "Writing Volume with dimensions: " << width << ", " << height << ", " << depth << std::endl;
    IG_LOG(L_INFO) << "Control coordinate (" << control_x << ", " << control_y << ", " << control_z << ") with value: " << grid->tree().getValue(nanovdb::Coord(control_x, control_y, control_z)) << std::endl;

    stream.write(reinterpret_cast<char*>(&width),  sizeof(width));
    stream.write(reinterpret_cast<char*>(&height), sizeof(height));
    stream.write(reinterpret_cast<char*>(&depth),  sizeof(depth));
    stream.write(reinterpret_cast<char*>(&zero),   sizeof(zero));
    
    //TODO: reset this to 1
    float scalar = 5.0f;


    for (uint32_t z = 0; z < depth; z++) {
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                auto value = static_cast<float>(grid->tree().getValue(nanovdb::Coord(x, y, z)));
                float sigma = value * scalar;
                stream.write(reinterpret_cast<char*>(&sigma), sizeof(float));
                stream.write(reinterpret_cast<char*>(&sigma), sizeof(float));
                stream.write(reinterpret_cast<char*>(&sigma), sizeof(float));
                stream.write(reinterpret_cast<char*>(&zero_f32), sizeof(float));
                stream.write(reinterpret_cast<char*>(&sigma), sizeof(float));
                stream.write(reinterpret_cast<char*>(&sigma), sizeof(float));
                stream.write(reinterpret_cast<char*>(&sigma), sizeof(float));
                stream.write(reinterpret_cast<char*>(&zero_f32), sizeof(float));
            }
        }
    }

    stream.close();
    return true;
}

} // Namespace IG
