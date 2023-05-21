#include "HeterogeneousMedium.h"
#include "loader/Parser.h"
#include "loader/ShadingTree.h"
#include "measured/NanoVDBLoader.h"
#include "loader/LoaderContext.h"
#include "Logger.h"
#include "loader/LoaderUtils.h"

#define USE_SPARSE_GRID


namespace IG {

#ifdef USE_SPARSE_GRID
static std::string setup_nvdb_grid(const std::filesystem::path path, const std::string medium_name, const std::string grid_name, LoaderContext& ctx)
{

    if (grid_name == "none") {
        return "";
    }

    const std::string exported_id = medium_name + "_" + grid_name + "_nvdb";

    const auto data = ctx.Cache->ExportedData.find(exported_id);
    if (data != ctx.Cache->ExportedData.end())
        return std::any_cast<std::string>(data->second);

    std::filesystem::create_directories("data/"); // Make sure this directory exists
    std::string out_path = "data/nvdb_" + medium_name + "_" + grid_name + ".bin";

    if (!NanoVDBLoader::prepare(path, grid_name, out_path))
        ctx.signalError();
    else {
        IG_LOG(L_INFO) << "Created sparse bin buffer file from NVDB " << grid_name << " grid." << std::endl;
    }
    ctx.Cache->ExportedData[exported_id] = out_path;
    return out_path;
}
#else
static std::string setup_uniform_nvdb_grid(const std::filesystem::path path, const std::string medium_name, LoaderContext& ctx)
{
    const std::string exported_id = medium_name + "_nvdb";

    const auto data = ctx.Cache->ExportedData.find(exported_id);
    if (data != ctx.Cache->ExportedData.end())
        return std::any_cast<std::string>(data->second);

    std::filesystem::create_directories("data/"); // Make sure this directory exists
    std::string out_path = "data/nvdb_" + medium_name + ".bin";

    if (!NanoVDBLoader::prepare_naive_grid(path, out_path))
        ctx.signalError();


    IG_LOG(L_INFO) << "Created naive bin buffer file from NVDB File." << std::endl;

    ctx.Cache->ExportedData[exported_id] = out_path;
    return out_path;
}
#endif

HeterogeneousMedium::HeterogeneousMedium(const std::string& name, const std::shared_ptr<SceneObject>& medium)
    : Medium(name, "heterogeneous")
    , mMedium(medium)
{
    handleReferenceEntity(*medium);
}

void HeterogeneousMedium::serialize(const SerializationInput& input) const
{
    input.Tree.beginClosure(name());
    const std::string media_id = input.Tree.currentClosureID();

    // full file path to Media file is given
    std::string full_path = mMedium->property("filename").getString();

    // find last path separator
    auto last_sep = full_path.find_last_of('/');
    // extract medium name by also getting rid of file extension (.nvdb or .bin)
    auto medium_name            = full_path.substr(last_sep + 1, full_path.length() - last_sep - 6);
    auto filename               = input.Tree.context().handlePath(full_path, *mMedium);
    const std::string extension = filename.extension().u8string();

    std::string buffer_name     = "buffer_" + medium_name;
    std::string volume_name     = "volume_" + medium_name; // the volume (data) of the volumetric media
    std::string generator_name  = "medium_" + media_id;    // the function used to generate the MediumSamples

    const bool interpolate = mMedium->property("interpolate").getBool(false);

    input.Tree.addNumber("g", *mMedium, 0.0f);
    const std::string pms_func = generateReferencePMS(input);

    if (extension == ".nvdb") {        
        // Principled Volume Shader Parameters
        std::string shader_name         = "pvs_" + medium_name;
        std::string shader_params       = "pvs_params_" + medium_name;
        const float scalar_density      = mMedium->property("scalar_density" ).getNumber(1.0f);
        const float scalar_emission     = mMedium->property("scalar_emission").getNumber(0.0f);
        const Vector3f color_scattering = mMedium->property("color_scattering").getVector3(Vector3f(0.5f, 0.5f, 0.5f));
        const Vector3f color_absorption = mMedium->property("color_absorption").getVector3(Vector3f(0.8f, 0.8f, 0.8f));
        const Vector3f color_emission   = mMedium->property("color_emission"  ).getVector3(Vector3f(1.0f, 1.0f, 1.0f));
        const Vector3f color_blackbody  = mMedium->property("color_blackbody" ).getVector3(Vector3f(0.0f, 0.0f, 0.0f)); //TODO: set defaults according to blender
        const float scalar_blackbody    = std::min(std::max(mMedium->property("scalar_blackbody" ).getNumber(1.0f), 0.0f), 1.0f); //clamped between 0 and 1
        const float scalar_temperature  = mMedium->property("scalar_temperature").getNumber(0.0f);
        const std::string grid_name_density     = mMedium->property("grid_density").getString("density");
        const std::string grid_name_temperature = mMedium->property("grid_temperature").getString("none");

#ifdef USE_SPARSE_GRID
        const auto bin_filename_density     = setup_nvdb_grid(filename, medium_name, grid_name_density, input.Tree.context());
        //TODO: check if grid_name_temparure is "none", if so, do not load the grid
        const auto bin_filename_temperature = setup_nvdb_grid(filename, medium_name, grid_name_temperature, input.Tree.context());
#else
        const auto bin_filename = setup_uniform_nvdb_grid(filename, medium_name, input.Tree.context());
#endif

        const std::string buffer_name_density     = buffer_name + "_density";
        const std::string buffer_name_temperature = buffer_name + "_temperature";
        size_t res_id_density = input.Tree.context().registerExternalResource(bin_filename_density);

        input.Stream << input.Tree.pullHeader()
            << "  let " << buffer_name_density << " = device.load_buffer_by_id(" << res_id_density << ");" << std::endl;

        if (grid_name_temperature == "none") {
            input.Stream << input.Tree.pullHeader()
                << "  let " << buffer_name_temperature << " = Option[DeviceBuffer]::None;" << std::endl;
        } else {
            size_t res_id_temperature = input.Tree.context().registerExternalResource(bin_filename_temperature);
            input.Stream << input.Tree.pullHeader()
                << "  let " << buffer_name_temperature << " = Option[DeviceBuffer]::Some(device.load_buffer_by_id(" << res_id_temperature << "));" << std::endl;
        }
        input.Stream << input.Tree.pullHeader()
        #ifdef USE_SPARSE_GRID
            << "  let " << shader_params  << " = make_principled_volume_parameters("
                                                    << scalar_density << ", "
                                                    << scalar_emission << ", "
                                                    << LoaderUtils::inlineColor(color_scattering) << ", "
                                                    << LoaderUtils::inlineColor(color_absorption) << ", "
                                                    << LoaderUtils::inlineColor(color_emission)   << ", "
                                                    << LoaderUtils::inlineColor(color_blackbody)  << ", "
                                                    << scalar_blackbody << ","
                                                    << scalar_temperature
                                                << ");"  << std::endl
            << "  let " << shader_name    << " = make_principled_volume_shader(" << shader_params << ");" << std::endl
            << "  let " << volume_name    << " = make_nvdb_volume_f32(" << buffer_name_density << ", " << buffer_name_temperature << ", " << shader_name << ");" << std::endl;
#else
            << "  let " << shader_name    << " = make_simple_volume_shader(1);" << std::endl
            << "  let " << volume_name    << " = make_voxel_grid(" << buffer_name << ", " << shader_name << ");" << std::endl;
#endif
    } else if (extension == ".bin") {

        std::string shader_name  = "vs_" + medium_name;
        size_t res_id = input.Tree.context().registerExternalResource(filename);
        const Vector3f majorant = mMedium->property("majorant").getVector3(Vector3f(10.0f, 10.0f, 10.0f));
        const float scalar = mMedium->property("scalar_density").getNumber(1.0f);

        input.Stream << input.Tree.pullHeader()
            << "  let " << buffer_name    << " = device.load_buffer_by_id(" << res_id << ");" << std::endl
            << "  let " << shader_name    << " = make_simple_volume_shader(" << scalar << ");" << std::endl
            << "  let " << volume_name    << " = make_voxel_grid(" << buffer_name << ", " << shader_name << ", " << LoaderUtils::inlineColor(majorant) << ");" << std::endl;
            //<< "  let " << volume_name    << " = make_vacuum_voxel_grid(1:f32);" << std::endl;
    } else {
        IG_LOG(L_ERROR) << "File extension " << extension << " for heterogeneous medium not supported" << std::endl;
        return;
    }

    const std::string method = mMedium->property("method").getString("regular");
    const int max_scattering = mMedium->property("max_scattering").getInteger(8);

    if (method == "delta_tracking") {
        const int majorant_dimension = mMedium->property("majorant_dimension").getInteger(128);
        input.Stream << "  let " << generator_name << ": MediumGenerator = @|ctx| { make_delta_tracking_medium(ctx, "<< pms_func << "(), " << volume_name << ", make_henyeygreenstein_phase(" << input.Tree.getInline("g") << "), " << (interpolate ? "true" : "false") << ", " << max_scattering << ", " << majorant_dimension << ") };" << std::endl;
    } else if (method == "ray_marching") {
        const float step_distance = mMedium->property("step_distance").getNumber(0.001f);
        input.Stream << "  let " << generator_name << ": MediumGenerator = @|ctx| { make_ray_marching_medium(ctx, "<< pms_func << "(), " << volume_name << ", make_henyeygreenstein_phase(" << input.Tree.getInline("g") << "), " << (interpolate ? "true" : "false") << ", " << max_scattering << ", " << step_distance << ") };" << std::endl;
    } else {
        input.Stream << "  let " << generator_name << ": MediumGenerator = @|ctx| { make_regular_tracking_medium(ctx, "<< pms_func << "(), " << volume_name << ", make_henyeygreenstein_phase(" << input.Tree.getInline("g") << "), " << (interpolate ? "true" : "false") << ", " << max_scattering << ") };" << std::endl;
    }
    input.Tree.endClosure();
}

} // namespace IG