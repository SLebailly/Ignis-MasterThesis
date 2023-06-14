#include "VolumePathTechnique.h"
#include "loader/LoaderContext.h"
#include "loader/LoaderLight.h"
#include "loader/Parser.h"
#include "loader/ShadingTree.h"
#include "shader/ShaderUtils.h"

#include "Logger.h"

namespace IG {
VolumePathTechnique::VolumePathTechnique(SceneObject& obj)
    : Technique("volpath")
{
    mMaxDepth      = obj.property("max_depth").getInteger(DefaultMaxRayDepth);
    mMinDepth      = obj.property("min_depth").getInteger(DefaultMinRayDepth);
    mLightSelector = obj.property("light_selector").getString();
    mClamp         = obj.property("clamp").getNumber(0.0f);
    mEnableNEE     = obj.property("nee").getBool(true);
    mEnableNEE     = obj.property("nee").getBool(true);
}

TechniqueInfo VolumePathTechnique::getInfo(const LoaderContext&) const
{
    TechniqueInfo info;
    info.Variants[0].UsesLights                = true;
    info.Variants[0].UsesMedia                 = true;
    info.Variants[0].PrimaryPayloadCount       = 7;
    info.Variants[0].EmitterPayloadInitializer = "make_simple_payload_initializer(init_vpt_raypayload)";

    return info;
}

void VolumePathTechnique::generateBody(const SerializationInput& input) const
{
    // Insert config into global registry
    input.Context.GlobalRegistry.IntParameters["__tech_max_depth"] = (int)mMaxDepth;
    input.Context.GlobalRegistry.IntParameters["__tech_min_depth"] = (int)mMinDepth;
    input.Context.GlobalRegistry.FloatParameters["__tech_clamp"]   = mClamp;

    if (mMaxDepth < 2) // 0 & 1 can be an optimization
        input.Stream << "  let tech_max_depth = " << mMaxDepth << ":i32;" << std::endl;
    else
        input.Stream << "  let tech_max_depth = registry::get_global_parameter_i32(\"__tech_max_depth\", 8);" << std::endl;

    if (mMinDepth < 2) // 0 & 1 can be an optimization
        input.Stream << "  let tech_min_depth = " << mMinDepth << ":i32;" << std::endl;
    else
        input.Stream << "  let tech_min_depth = registry::get_global_parameter_i32(\"__tech_min_depth\", 2);" << std::endl;

    if (mClamp <= 0) // 0 is a special case
        input.Stream << "  let tech_clamp = " << mClamp << ":f32;" << std::endl;
    else
        input.Stream << "  let tech_clamp = registry::get_global_parameter_f32(\"__tech_clamp\", 0);" << std::endl;

    IG_LOG(L_DEBUG) << "Starting volume path renderer with NEE set to " << (mEnableNEE ? "true" : "false") << "." << std::endl;
    ShadingTree tree(input.Context);
    input.Stream << input.Context.Lights->generateLightSelector(mLightSelector, tree)
                 << "  let framebuffer = device.load_aov_image(\"\", " << ShaderUtils::inlineSPI(input.Context) << ");"
                 << "  let technique = make_volume_path_renderer(tech_max_depth, light_selector, media, framebuffer, tech_clamp, " 
                 << (mEnableNEE ? "true" : "false") << ");" << std::endl;
}

} // namespace IG