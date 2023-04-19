#include "VolumeLight.h"
#include "Logger.h"
#include "loader/LoaderEntity.h"
#include "loader/LoaderShape.h"
#include "loader/LoaderUtils.h"
#include "loader/Parser.h"
#include "loader/ShadingTree.h"
#include "serialization/VectorSerializer.h"
#include "table/SceneDatabase.h"

namespace IG {

VolumeLight::VolumeLight(const std::string& name, const LoaderContext& ctx, const std::shared_ptr<SceneObject>& light)
    : Light(name, light->pluginType())
    , mLight(light)
{
    mEntity   = light->property("entity").getString();
    mMedium = light->property("inner_medium").getString();
}

void VolumeLight::serialize(const SerializationInput& input) const
{
    input.Tree.beginClosure(name());

    const auto entity = input.Tree.context().Entities->getEmissiveEntity(mEntity);
    if (!entity.has_value()) {
        IG_LOG(L_ERROR) << "No entity named '" << mEntity << "' exists for area light" << std::endl;
        return;
    }

    const std::string light_id = input.Tree.currentClosureID();
    input.Stream << input.Tree.pullHeader();

    const std::string media_id = input.Tree.getClosureID(name);

    const auto& shape = input.Tree.context().Shapes->getShape(entity->ShapeID);
    const auto& tri   = input.Tree.context().Shapes->getTriShape(entity->ShapeID);
    input.Stream << "  let trimesh_" << light_id << " = load_trimesh_entry(device, "
                    << shape.TableOffset
                    << ", " << tri.FaceCount
                    << ", " << tri.VertexCount
                    << ", " << tri.NormalCount
                    << ", " << tri.TexCount << ");" << std::endl
                    << "  let ae_" << light_id << " = make_shape_area_emitter(" << LoaderUtils::inlineEntity(*entity)
                    << ", make_trimesh_shape(trimesh_" << light_id << ")"
                    << ", trimesh_" << light_id << ");" << std::endl;

    }

    input.Stream << "  let light_" << light_id << " = make_area_light(" << input.ID
                 << ", ae_" << light_id;

    input.Stream << ", @|ctx| { maybe_unused(ctx); " << input.Tree.getInline("radiance") << " });" << std::endl;

    input.Tree.endClosure();
}

} // namespace IG