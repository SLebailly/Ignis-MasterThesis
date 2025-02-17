#include "HeterogeneousMedium.h"
#include "loader/Parser.h"
#include "loader/ShadingTree.h"
#include "measured/NanoVDBLoader.h"
#include "loader/LoaderContext.h"
#include "Logger.h"
#include "loader/LoaderUtils.h"


namespace IG {


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

static std::string setup_uniform_nvdb_grid(const std::filesystem::path path, const std::string medium_name, const std::string grid_name_density, const std::string grid_name_temperature, LoaderContext& ctx)
{
    const std::string exported_id = medium_name + "_nvdb";

    const auto data = ctx.Cache->ExportedData.find(exported_id);
    if (data != ctx.Cache->ExportedData.end())
        return std::any_cast<std::string>(data->second);

    std::filesystem::create_directories("data/"); // Make sure this directory exists
    std::string out_path = "data/nvdb_" + medium_name + ".bin";

    if (!NanoVDBLoader::prepare_uniform_grid(path, out_path, grid_name_density, grid_name_temperature))
        ctx.signalError();


    IG_LOG(L_INFO) << "Created uniform bin buffer file from NVDB File." << std::endl;

    ctx.Cache->ExportedData[exported_id] = out_path;
    return out_path;
}

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

    const bool interpolate      = mMedium->property("interpolate").getBool(false);

    input.Tree.addNumber("g", *mMedium, 0.0f);
    const std::string pms_func = generateReferencePMS(input);

    if (extension == ".nvdb") {
        std::string shader              = mMedium->property("shader").getString("monochromatic");
        std::string shader_name         = "shader_" + medium_name;
        if (shader == "principled_volume") {
            // Principled Volume Shader Parameters
            std::string shader_params       = "pvs_params_" + medium_name;
            const float scalar_density      = mMedium->property("scalar_density" ).getNumber(1.0f);
            const float scalar_emission     = mMedium->property("scalar_emission").getNumber(0.0f);
            const Vector3f color_scattering = mMedium->property("color_scattering").getVector3(Vector3f(0.5f, 0.5f, 0.5f));
            const Vector3f color_absorption = mMedium->property("color_absorption").getVector3(Vector3f(0.8f, 0.8f, 0.8f));
            const Vector3f color_emission   = mMedium->property("color_emission"  ).getVector3(Vector3f(1.0f, 1.0f, 1.0f));
            const Vector3f color_blackbody  = mMedium->property("color_blackbody" ).getVector3(Vector3f(0.0f, 0.0f, 0.0f)); //TODO: set defaults according to blender
            const float scalar_blackbody    = std::min(std::max(mMedium->property("scalar_blackbody" ).getNumber(1.0f), 0.0f), 1.0f); //clamped between 0 and 1
            const float scalar_temperature  = mMedium->property("scalar_temperature").getNumber(0.0f);

            input.Stream << input.Tree.pullHeader()
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
            	         << "  let " << shader_name << " = make_principled_volume_shader(" << shader_params << ");" << std::endl;
        } else if (shader == "pbrt_volume") {
            // PBRT Volume Shader Parameters
            std::string shader_params       = "pbrtv_params_" + medium_name;
            const float scalar_density      = mMedium->property("scalar_density" ).getNumber(1.0f);
            const float scalar_emission     = mMedium->property("scalar_emission").getNumber(0.0f);
            const float scalar_temperature  = mMedium->property("scalar_temperature").getNumber(0.0f);
            const Vector3f color_scattering = mMedium->property("color_scattering").getVector3(Vector3f(0.5f, 0.5f, 0.5f));
            const Vector3f color_absorption = mMedium->property("color_absorption").getVector3(Vector3f(0.8f, 0.8f, 0.8f));
            const Vector3f color_emission   = mMedium->property("color_emission"  ).getVector3(Vector3f(1.0f, 1.0f, 1.0f));
            const float cutoff_temperature  = mMedium->property("cutoff_temperature").getNumber(0.0f);
            const float offset_temperature  = mMedium->property("offset_temperature").getNumber(cutoff_temperature);

            input.Stream << input.Tree.pullHeader()
                         << "  let " << shader_params  << " = make_pbrt_volume_parameters("
                            << scalar_density << ", "
                            << scalar_emission << ", "
                            << LoaderUtils::inlineColor(color_scattering) << ", "
                            << LoaderUtils::inlineColor(color_absorption) << ", "
                            << LoaderUtils::inlineColor(color_emission)   << ", "
                            << scalar_temperature << ", "
                            << offset_temperature
                         << ");"  << std::endl
            	         << "  let " << shader_name << " = make_pbrt_volume_shader(" << shader_params << ");" << std::endl;
        } else {
            // monochromatic
            std::string shader_params       = "mono_params_" + medium_name;
            const float scalar_density      = mMedium->property("scalar_density").getNumber(1.0f);
            const float scalar_absorption   = mMedium->property("scalar_absorption").getNumber(1.0f);
            const float scalar_scattering   = mMedium->property("scalar_scattering").getNumber(1.0f);
            input.Stream << input.Tree.pullHeader()
                << "  let " << shader_params  << " = make_monochromatic_volume_parameters("
                << scalar_density << ", "
                << scalar_absorption << ", "
                << scalar_scattering
                << ");"  << std::endl
                << "  let " << shader_name << " = make_monochromatic_volume_shader(" << shader_params << ");" << std::endl;
        }
        
        const std::string grid_name_density     = mMedium->property("grid_density").getString("density");
        const std::string grid_name_temperature = mMedium->property("grid_temperature").getString("none");

        const std::string grid_type             = mMedium->property("grid_type").getString("sparse");

        std::string bin_filename_density;
        std::string bin_filename_temperature;
        if (grid_type == "uniform") {
            bin_filename_density = setup_uniform_nvdb_grid(filename, medium_name, grid_name_density, grid_name_temperature, input.Tree.context());
        } else {
            bin_filename_density     = setup_nvdb_grid(filename, medium_name, grid_name_density, input.Tree.context());
            //TODO: check if grid_name_temparure is "none", if so, do not load the grid
            bin_filename_temperature = setup_nvdb_grid(filename, medium_name, grid_name_temperature, input.Tree.context());
        }

        const std::string buffer_name_density     = buffer_name + "_density";
        const std::string buffer_name_temperature = buffer_name + "_temperature";
        size_t res_id_density = input.Tree.context().registerExternalResource(bin_filename_density);

        input.Stream << input.Tree.pullHeader()
            << "  let " << buffer_name_density << " = device.load_buffer_by_id(" << res_id_density << ");" << std::endl;



        if (grid_type == "uniform") {
            const Vector3f majorant = mMedium->property("majorant").getVector3(Vector3f(100.0f, 100.0f, 100.0f));
            input.Stream << input.Tree.pullHeader()
            << "  let " << volume_name    << " = make_density_grid(" << buffer_name_density << ", " << shader_name << ", " << LoaderUtils::inlineColor(majorant) << ");" << std::endl;
        } else {
            if (grid_name_temperature == "none") {
                input.Stream << input.Tree.pullHeader()
                    << "  let " << buffer_name_temperature << " = Option[DeviceBuffer]::None;" << std::endl;
            } else {
                size_t res_id_temperature = input.Tree.context().registerExternalResource(bin_filename_temperature);
                input.Stream << input.Tree.pullHeader()
                    << "  let " << buffer_name_temperature << " = make_option(device.load_buffer_by_id(" << res_id_temperature << "));" << std::endl;
            }
            input.Stream << input.Tree.pullHeader()
            << "  let " << volume_name    << " = make_nvdb_volume_f32(" << buffer_name_density << ", " << buffer_name_temperature << ", " << shader_name << ");" << std::endl;
        }
    } else if (extension == ".bin") {

        std::string shader_name  = "vs_" + medium_name;
        size_t res_id = input.Tree.context().registerExternalResource(filename);
        const Vector3f majorant = mMedium->property("majorant").getVector3(Vector3f(10.0f, 10.0f, 10.0f));
        
        const float scalar_density      = mMedium->property("scalar_density").getNumber(1.0f);
        const float scalar_absorption   = mMedium->property("scalar_absorption").getNumber(1.0f);
        const float scalar_scattering   = mMedium->property("scalar_scattering").getNumber(1.0f);
        const float scalar_emission     = mMedium->property("scalar_emission").getNumber(1.0f);
        const Vector3f color_scattering = mMedium->property("color_scattering").getVector3(Vector3f(1.0f, 1.0f, 1.0f));
        const Vector3f color_absorption = mMedium->property("color_absorption").getVector3(Vector3f(1.0f, 1.0f, 1.0f));
        const Vector3f color_emission   = mMedium->property("color_emission"  ).getVector3(Vector3f(1.0f, 1.0f, 1.0f));

        const Vector3f scalar_color_scattering = Vector3f(color_scattering[0] * scalar_density * scalar_scattering, color_scattering[1] * scalar_density * scalar_scattering, color_scattering[2] * scalar_density * scalar_scattering);
        const Vector3f scalar_color_absorption = Vector3f(color_absorption[0] * scalar_density * scalar_absorption, color_absorption[1] * scalar_density * scalar_absorption, color_absorption[2] * scalar_density * scalar_absorption);
        const Vector3f scalar_color_emission   = Vector3f(color_emission[0]   * scalar_density * scalar_emission,   color_emission[1]   * scalar_density * scalar_emission,   color_emission[2]   * scalar_density * scalar_emission);

        input.Stream << input.Tree.pullHeader()
            << "  let " << buffer_name    << " = device.load_buffer_by_id(" << res_id << ");" << std::endl
            << "  let " << shader_name    << " = make_simple_volume_shader(" << LoaderUtils::inlineColor(scalar_color_scattering) << ", " << LoaderUtils::inlineColor(scalar_color_absorption) << ", " << LoaderUtils::inlineColor(scalar_color_emission) << ");" << std::endl
            << "  let " << volume_name    << " = make_uniform_grid(" << buffer_name << ", " << shader_name << ", " << LoaderUtils::inlineColor(majorant) << ");" << std::endl;
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
        const std::string marcher = mMedium->property("marcher").getString("DDA");
        input.Stream << "  let " << generator_name << ": MediumGenerator = @|ctx| { make_regular_tracking_medium(ctx, "<< pms_func << "(), " << volume_name << ", make_henyeygreenstein_phase(" << input.Tree.getInline("g") << "), " << (interpolate ? "true" : "false") << ", " << max_scattering << ", " << (marcher == "DDA" ? "make_dda_marcher" : "make_hdda_marcher") << ") };" << std::endl;
    }
    input.Tree.endClosure();
}

} // namespace IG