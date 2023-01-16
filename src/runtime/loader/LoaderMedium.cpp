#include "LoaderMedium.h"
#include "Loader.h"
#include "LoaderUtils.h"
#include "Logger.h"
#include "ShadingTree.h"
#include "measured/NanoVDBLoader.h"
#include "medium/HomogeneousMedium.h"
#include "medium/VacuumMedium.h"

#define USE_SPARSE_GRID

namespace IG {


#ifdef USE_SPARSE_GRID
static std::string setup_nvdb_grid(const std::string& name, const std::filesystem::path path, LoaderContext& ctx) //look here
{
    //auto filename = ctx.handlePath(medium->property("filename").getString(), *medium);

    const std::string exported_id = "_nvdb_" + path.u8string();

    const auto data = ctx.ExportedData.find(exported_id);

    
    if (data != ctx.ExportedData.end())
        return std::any_cast<std::string>(data->second);
    

    std::filesystem::create_directories("data/"); // Make sure this directory exists
    std::string out_path = "data/nvdb_" + LoaderUtils::escapeIdentifier(name) + "_" + path.filename().u8string() + ".bin";

    if (!NanoVDBLoader::prepare(path, out_path))
        ctx.signalError();
    else {
        IG_LOG(L_INFO) << "Created sparse bin buffer file from NVDB File." << std::endl;
    }
    ctx.ExportedData[exported_id] = out_path;
    return out_path;
}
#else
static std::string setup_naive_nvdb_grid(const std::string& name, const std::filesystem::path path, LoaderContext& ctx) //look here
{
    //auto filename = ctx.handlePath(medium->property("filename").getString(), *medium);

    const std::string exported_id = "_nvdb_" + path.u8string();

    const auto data = ctx.ExportedData.find(exported_id);

    
    if (data != ctx.ExportedData.end())
        return std::any_cast<std::string>(data->second);
    

    std::filesystem::create_directories("data/"); // Make sure this directory exists
    std::string out_path = "data/nvdb_" + LoaderUtils::escapeIdentifier(name) + "_" + path.filename().u8string() + ".bin";

    if (!NanoVDBLoader::prepare_naive_grid(path, out_path))
        ctx.signalError();


    IG_LOG(L_INFO) << "Created naive bin buffer file from NVDB File." << std::endl;

    ctx.ExportedData[exported_id] = out_path;
    return out_path;
}
#endif


#ifdef USE_SPARSE_GRID
static std::string setup_nvdb_grid(const std::string& name, const std::filesystem::path path, LoaderContext& ctx) //look here
{
    //auto filename = ctx.handlePath(medium->property("filename").getString(), *medium);

    const std::string exported_id = "_nvdb_" + path.u8string();

    const auto data = ctx.ExportedData.find(exported_id);

    
    if (data != ctx.ExportedData.end())
        return std::any_cast<std::string>(data->second);
    

    std::filesystem::create_directories("data/"); // Make sure this directory exists
    std::string out_path = "data/nvdb_" + LoaderUtils::escapeIdentifier(name) + "_" + path.filename().u8string() + ".bin";

    if (!NanoVDBLoader::prepare(path, out_path))
        ctx.signalError();
    else {
        IG_LOG(L_INFO) << "Created sparse bin buffer file from NVDB File." << std::endl;
    }
    ctx.ExportedData[exported_id] = out_path;
    return out_path;
}
#else
static std::string setup_naive_nvdb_grid(const std::string& name, const std::filesystem::path path, LoaderContext& ctx) //look here
{
    //auto filename = ctx.handlePath(medium->property("filename").getString(), *medium);

    const std::string exported_id = "_nvdb_" + path.u8string();

    const auto data = ctx.ExportedData.find(exported_id);

    
    if (data != ctx.ExportedData.end())
        return std::any_cast<std::string>(data->second);
    

    std::filesystem::create_directories("data/"); // Make sure this directory exists
    std::string out_path = "data/nvdb_" + LoaderUtils::escapeIdentifier(name) + "_" + path.filename().u8string() + ".bin";

    if (!NanoVDBLoader::prepare_naive_grid(path, out_path))
        ctx.signalError();


    IG_LOG(L_INFO) << "Created naive bin buffer file from NVDB File." << std::endl;

    ctx.ExportedData[exported_id] = out_path;
    return out_path;
}
#endif

static std::shared_ptr<Medium> medium_homogeneous(const std::string& name, const std::shared_ptr<SceneObject>& medium, LoaderContext&)
{
    return std::make_shared<HomogeneousMedium>(name, medium);
}

static void medium_heterogeneous(std::ostream& stream, const std::string& name, const std::shared_ptr<Parser::Object>& medium, ShadingTree& tree)
{
    tree.beginClosure(name);

    const auto filename    = tree.context().handlePath(medium->property("filename").getString(), *medium);
    const bool interpolate = medium->property("interpolate").getBool(false);

    tree.addNumber("g", *medium, 0, true);

    const std::string extension = filename.extension().u8string();
    if (extension == ".nvdb") {
#ifdef USE_SPARSE_GRID
        const auto bin_filename = setup_nvdb_grid(name, filename, tree.context());
        size_t res_id = tree.context().registerExternalResource(bin_filename);
        
        // Principled Volume Shader Parameters
        const float scalar_density   = medium->property("scalar_density" ).getNumber(1.0f);
        const float scalar_emission  = medium->property("scalar_emission").getNumber(0.0f);
        const Vector3f color_scattering = medium->property("color_scattering").getVector3(Vector3f(0.5f, 0.5f, 0.5f));
        const Vector3f color_absorption = medium->property("color_absorption").getVector3(Vector3f(0.8f, 0.8f, 0.8f));
        const Vector3f color_emission   = medium->property("color_emission"  ).getVector3(Vector3f(1.0f, 1.0f, 1.0f));
        
        const std::string media_id = tree.currentClosureID();

        stream << tree.pullHeader()
            << "  let shader_" << media_id << " = make_principled_volume_shader("
            << "      make_principled_volume_parameters(" << scalar_density << ", " << scalar_emission << ", " << LoaderUtils::inlineColor(color_scattering) << ", " << LoaderUtils::inlineColor(color_absorption) << ", " << LoaderUtils::inlineColor(color_emission) << ")"
            << "  );" << std::endl
            << "  let medium_" << media_id << "_buffer = device.load_buffer_by_id(" << res_id << ");" << std::endl
            << "  let medium_" << media_id << "_volume = make_nvdb_volume_f32(medium_" << media_id << "_buffer, shader_" << media_id << ");" << std::endl
            << "  let medium_" << media_id << " : MediumGenerator= @|ctx| { make_heterogeneous_medium(ctx, "<< "medium_" << media_id << "_volume"
            << ", make_henyeygreenstein_phase(" << tree.getInline("g") << "), " << (interpolate ? "true" : "false") << ") };" << std::endl;

        tree.endClosure();
        return;
#else
        const auto bin_filename = setup_naive_nvdb_grid(name, filename, tree.context());
        size_t res_id = tree.context().registerExternalResource(bin_filename);
        
        const std::string media_id = tree.currentClosureID();
        stream << tree.pullHeader()
            << "  let medium_" << media_id << "_grid = make_voxel_grid(device.load_buffer_by_id(" << res_id << "));" << std::endl
            << "  let medium_" << media_id << " : MediumGenerator= @|ctx| { make_heterogeneous_medium(ctx, "<< "medium_" << media_id << "_grid"
            << ", make_henyeygreenstein_phase(" << tree.getInline("g") << "), " << (interpolate ? "true" : "false") << ") };" << std::endl;
            
        tree.endClosure();
        return;
#endif
    } else if (extension == ".bin") {

        size_t res_id = tree.context().registerExternalResource(filename);

        const std::string media_id = tree.currentClosureID();
        stream << tree.pullHeader()
            << "  let medium_" << media_id << "_volume = make_voxel_grid(device.load_buffer_by_id(" << res_id << "), make_simple_volume_shader(1));" << std::endl
            << "  let medium_" << media_id << " : MediumGenerator= @|ctx| { make_heterogeneous_medium(ctx, "<< "medium_" << media_id << "_volume"
            << ", make_henyeygreenstein_phase(" << tree.getInline("g") << "), " << (interpolate ? "true" : "false") << ") };" << std::endl;

        tree.endClosure();

        return;
    } else {
        IG_LOG(L_ERROR) << "File extension " << extension << " for heterogeneous medium not supported" << std::endl;
    }


}

// It is recommended to not define the medium, instead of using vacuum
static std::shared_ptr<Medium> medium_vacuum(const std::string& name, const std::shared_ptr<SceneObject>&, LoaderContext&)
{
    return std::make_shared<VacuumMedium>(name);
}

using MediumLoader = std::shared_ptr<Medium> (*)(const std::string&, const std::shared_ptr<SceneObject>&, LoaderContext&);
static const struct {
    const char* Name;
    MediumLoader Loader;
} _generators[] = {
    { "homogeneous", medium_homogeneous },
    { "heterogeneous", medium_heterogeneous },
    { "constant", medium_homogeneous },
    { "vacuum", medium_vacuum },
    { "", nullptr }
};


void LoaderMedium::loadMedia(LoaderContext& ctx)
{
    size_t id = 0;
    for (const auto& pair : ctx.Options.Scene->media()) {
        const auto medium = pair.second;

        bool found = false;
        for (size_t i = 0; _generators[i].Loader; ++i) {
            if (_generators[i].Name == medium->pluginType()) {
                auto ptr = _generators[i].Loader(pair.first, medium, ctx);
                ptr->setID(id++);
                mMedia[pair.first] = ptr;
                found              = true;
                break;
            }
        }
        if (!found)
            IG_LOG(L_ERROR) << "No medium type '" << medium->pluginType() << "' available" << std::endl;
    }
}

void LoaderMedium::handleReferenceEntity(const std::string& mediumInnerName, const std::string& entity_name, size_t id)
{
    // TODO: This can be a bottle neck if many different media is used (which is unlikely the case though)
    for (auto pair : mMedia) {
        auto medium = pair.second;
        if (medium->hasReferenceEntity() && medium->referenceEntity() == entity_name) {
            medium->setReferenceEntityID(id);
        } else if (pair.first == mediumInnerName && !medium->hasReferenceEntity() && !medium->isReferenceEntityIDSet()) {
            medium->setReferenceEntityID(id);
            medium->setReferenceEntity(entity_name);
        }
    }
}

void LoaderMedium::prepare(LoaderContext& ctx)
{
    if (ctx.Options.Scene->media().empty())
        return;

    IG_LOG(L_DEBUG) << "Prepare participating media" << std::endl;
    loadMedia(ctx);
}

std::string LoaderMedium::generate(ShadingTree& tree)
{
    std::stringstream stream;

    size_t counter = 0;
    for (auto name : mAcquiredMedia)
        mMedia.at(name)->serialize(Medium::SerializationInput{ counter++, stream, tree });

    if (counter != 0)
        stream << std::endl;

    stream << "  let media = @|id:i32| {" << std::endl
           << "    match(id) {" << std::endl;

    counter = 0;
    for (const auto& name : mAcquiredMedia) {
        const std::string media_id = tree.getClosureID(name);
        stream << "      " << counter << " => medium_" << media_id
               << "," << std::endl;
        ++counter;
    }

    stream << "    _ => @|_ctx : ShadingContext| make_vacuum_medium()" << std::endl;

    stream << "    }" << std::endl
           << "  };" << std::endl;

    return stream.str();
}

size_t LoaderMedium::acquire(const std::string& name)
{
    if (auto it = std::find(mAcquiredMedia.begin(), mAcquiredMedia.end(), name); it != mAcquiredMedia.end())
        return std::distance(mAcquiredMedia.begin(), it);

    mAcquiredMedia.push_back(name);
    return mAcquiredMedia.size() - 1;
}
} // namespace IG