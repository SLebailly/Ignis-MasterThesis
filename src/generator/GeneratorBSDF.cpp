#include "GeneratorBSDF.h"
#include "Logger.h"

namespace IG {

constexpr float AIR_IOR	   = 1.000277f;
constexpr float GLASS_IOR  = 1.55f;
constexpr float RUBBER_IOR = 1.49f;

static void setup_microfacet(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	std::string alpha_u, alpha_v;
	if (bsdf->property("alpha_u").isValid()) {
		alpha_u = ctx.extractMaterialPropertyNumber(bsdf, "alpha_u", 0.1f);
		alpha_v = ctx.extractMaterialPropertyNumber(bsdf, "alpha_v", 0.1f);
	} else {
		alpha_u = ctx.extractMaterialPropertyNumber(bsdf, "alpha", 0.1f);
		alpha_v = alpha_u;
	}

	os << "make_beckmann_distribution(math, surf, " << alpha_u << ", " << alpha_v << ")";
}

static void bsdf_error(const std::string& msg, std::ostream& os)
{
	IG_LOG(L_ERROR) << msg << std::endl;
	os << "make_black_bsdf()/* ERROR */";
}

static void bsdf_diffuse(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	os << "make_diffuse_bsdf(math, surf, " << ctx.extractMaterialPropertyColor(bsdf, "reflectance") << ")";
}

static void bsdf_orennayar(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	os << "make_rough_diffuse_bsdf(math, surf, "
	   << ctx.extractMaterialPropertyNumber(bsdf, "alpha", 0.0f) << ", "
	   << ctx.extractMaterialPropertyColor(bsdf, "reflectance")
	   << ")";
}

static void bsdf_dielectric(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	os << "make_glass_bsdf(math, surf, "
	   << ctx.extractMaterialPropertyNumber(bsdf, "ext_ior", AIR_IOR) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "int_ior", GLASS_IOR) << ", "
	   << ctx.extractMaterialPropertyColor(bsdf, "specular_reflectance", 1.0f) << ", "
	   << ctx.extractMaterialPropertyColor(bsdf, "specular_transmittance", 1.0f) << ")";
}

static void bsdf_thindielectric(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	os << "make_thinglass_bsdf(math, surf, "
	   << ctx.extractMaterialPropertyNumber(bsdf, "ext_ior", AIR_IOR) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "int_ior", GLASS_IOR) << ", "
	   << ctx.extractMaterialPropertyColor(bsdf, "specular_reflectance", 1.0f) << ", "
	   << ctx.extractMaterialPropertyColor(bsdf, "specular_transmittance", 1.0f) << ")";
}

static void bsdf_mirror(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	os << "make_mirror_bsdf(math, surf, "
	   << ctx.extractMaterialPropertyColor(bsdf, "specular_reflectance", 1.0f) << ")";
}

static void bsdf_conductor(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	os << "make_conductor_bsdf(math, surf, "
	   << ctx.extractMaterialPropertyNumber(bsdf, "eta", 0.63660f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "k", 2.7834f) << ", " // TODO: Better defaults?
	   << ctx.extractMaterialPropertyColor(bsdf, "specular_reflectance", 1.0f) << ")";
}

static void bsdf_rough_conductor(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	os << "make_rough_conductor_bsdf(math, surf, "
	   << ctx.extractMaterialPropertyNumber(bsdf, "eta", 0.63660f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "k", 2.7834f) << ", " // TODO: Better defaults?
	   << ctx.extractMaterialPropertyColor(bsdf, "specular_reflectance", 1.0f) << ", ";
	setup_microfacet(bsdf, ctx, os);
	os << ")";
}

static void bsdf_plastic(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	os << "make_plastic_bsdf(math, surf, "
	   << ctx.extractMaterialPropertyNumber(bsdf, "ext_ior", AIR_IOR) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "int_ior", RUBBER_IOR) << ", "
	   << ctx.extractMaterialPropertyColor(bsdf, "specular_reflectance", 1.0f) << ", "
	   << ctx.extractMaterialPropertyColor(bsdf, "diffuse_reflectance", 0.5f) << ")";
}

static void bsdf_phong(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	os << "make_phong_bsdf(math, surf, "
	   << ctx.extractMaterialPropertyColor(bsdf, "specular_reflectance", 1.0f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "exponent", 30) << ")";
}

static void bsdf_disney(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	os << "make_disney_bsdf(math, surf, "
	   << ctx.extractMaterialPropertyColor(bsdf, "base_color", 0.8f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "flatness", 0.0f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "metallic", 0.0f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "ior", GLASS_IOR) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "specular_tint", 0.0f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "roughness", 0.5f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "anisotropic", 0.0f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "sheen", 0.0f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "sheen_tint", 0.0f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "clearcoat", 0.0f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "clearcoat_gloss", 0.0f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "spec_trans", 0.0f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "relative_ior", 1.1f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "scatter_distance", 0.5f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "diff_trans", 0.0f) << ", "
	   << ctx.extractMaterialPropertyNumber(bsdf, "transmittance", 1.0f) << ")";
}

static void bsdf_blend(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	const std::string first	 = bsdf->property("first").getString();
	const std::string second = bsdf->property("second").getString();

	if (first.empty() || second.empty()) {
		bsdf_error("Invalid blend bsdf", os);
	} else if (first == second) {
		os << GeneratorBSDF::extract(ctx.Scene.bsdf(first), ctx);
	} else {
		os << GeneratorBSDF::extract(ctx.Scene.bsdf(first), ctx) << ", "
		   << GeneratorBSDF::extract(ctx.Scene.bsdf(second), ctx) << ", "
		   << ctx.extractMaterialPropertyNumber(bsdf, "weight", 0.5f) << ")";
	}
}

static void bsdf_mask(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	const std::string masked = bsdf->property("bsdf").getString();

	if (masked.empty())
		bsdf_error("Invalid mask bsdf", os);
	else
		os << "make_mix_bsdf(make_passthrough_bsdf(surf), "
		   << GeneratorBSDF::extract(ctx.Scene.bsdf(masked), ctx) << ", "
		   << ctx.extractMaterialPropertyNumber(bsdf, "opacity", 0.5f) << ")";
}

static void bsdf_twosided(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	/* Ignore */
	const std::string other = bsdf->property("bsdf").getString();

	if (other.empty())
		bsdf_error("Invalid twosided bsdf", os);
	else
		os << GeneratorBSDF::extract(ctx.Scene.bsdf(other), ctx);
}

static void bsdf_passthrough(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os)
{
	os << "make_passthrough_bsdf(surf)";
}

static void bsdf_null(const std::shared_ptr<Loader::Object>&, const GeneratorContext&, std::ostream& os)
{
	os << "make_black_bsdf()/* Null */";
}

static void bsdf_unknown(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext&, std::ostream& os)
{
	IG_LOG(L_WARNING) << "Unknown bsdf '" << bsdf->pluginType() << "'" << std::endl;
	os << "make_black_bsdf()/* Unknown " << bsdf->pluginType() << " */";
}

using BSDFGenerator = void (*)(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx, std::ostream& os);
static struct {
	const char* Name;
	BSDFGenerator Generator;
} _generators[] = {
	{ "diffuse", bsdf_diffuse },
	{ "roughdiffuse", bsdf_orennayar },
	{ "glass", bsdf_dielectric },
	{ "dielectric", bsdf_dielectric },
	{ "roughdielectric", bsdf_dielectric }, /*TODO*/
	{ "thindielectric", bsdf_thindielectric },
	{ "mirror", bsdf_mirror }, // Specialized conductor
	{ "conductor", bsdf_conductor },
	{ "roughconductor", bsdf_rough_conductor },
	{ "phong", bsdf_phong },
	{ "disney", bsdf_disney },
	{ "plastic", bsdf_plastic },
	{ "roughplastic", bsdf_plastic }, /*TODO*/
	{ "blendbsdf", bsdf_blend },
	{ "mask", bsdf_mask },
	{ "twosided", bsdf_twosided },
	{ "passthrough", bsdf_passthrough },
	{ "null", bsdf_null },
	{ "", nullptr }
};

std::string GeneratorBSDF::extract(const std::shared_ptr<Loader::Object>& bsdf, const GeneratorContext& ctx)
{
	std::stringstream sstream;

	if (!bsdf) {
		bsdf_error("No bsdf given", sstream);
	} else {
		for (size_t i = 0; _generators[i].Generator; ++i) {
			if (_generators[i].Name == bsdf->pluginType()) {
				_generators[i].Generator(bsdf, ctx, sstream);
				return sstream.str();
			}
		}
		bsdf_unknown(bsdf, ctx, sstream);
	}

	return sstream.str();
}
} // namespace IG
