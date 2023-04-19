#pragma once

#include "Light.h"

namespace IG {
class LoaderContext;

class VolumeLight : public Light {
public:
    VolumeLight(const std::string& name, const LoaderContext& ctx, const std::shared_ptr<SceneObject>& light);

    virtual std::optional<std::string> entity() const override { return mEntity; }
    
    virtual void serialize(const SerializationInput& input) const override;

    virtual std::optional<std::string> getEmbedClass() const override;
    virtual void embed(const EmbedInput& input) const override;

private:
    IG::BoundingBox mBBox;
    std::string mEntity;
    std::string mMedium;
    std::shared_ptr<SceneObject> mLight;
};
} // namespace IG