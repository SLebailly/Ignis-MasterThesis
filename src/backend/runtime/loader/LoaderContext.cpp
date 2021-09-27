#include "LoaderContext.h"
#include "Image.h"
#include "Logger.h"
#include "serialization/VectorSerializer.h"
#include "table/SceneDatabase.h"

namespace IG {

using namespace Parser;

bool LoaderContext::isTexture(const std::shared_ptr<Parser::Object>& obj, const std::string& propname) const
{
	auto prop = obj->property(propname);
	return prop.isValid() && prop.type() == PT_STRING;
}

Vector3f LoaderContext::extractColor(const std::shared_ptr<Parser::Object>& obj, const std::string& propname, const Vector3f& def) const
{
	auto prop = obj->property(propname);
	if (prop.isValid()) {
		switch (prop.type()) {
		case PT_INTEGER:
			return Vector3f::Ones() * prop.getInteger();
		case PT_NUMBER:
			return Vector3f::Ones() * prop.getNumber();
		case PT_VECTOR3:
			return prop.getVector3();
		case PT_STRING: {
			std::string name = obj->property(propname).getString();
			IG_LOG(L_WARNING) << "[TODO] Replacing texture '" << name << "' by average color" << std::endl;
			if (TextureBuffer.count(name)) {
				uint32 id = TextureBuffer.at(name);
				return TextureAverages.at(id);
			} else {
				IG_LOG(L_ERROR) << "No texture '" << name << "' found!" << std::endl;
				return def;
			}
		}
		default:
			IG_LOG(L_WARNING) << "Unknown property type for '" << propname << "'" << std::endl;
			return def;
		}
	} else {
		return def;
	}
}

float LoaderContext::extractIOR(const std::shared_ptr<Parser::Object>& obj, const std::string& propname, float def) const
{
	auto prop = obj->property(propname);
	if (prop.isValid()) {
		switch (prop.type()) {
		case PT_INTEGER:
			return prop.getInteger();
		case PT_NUMBER:
			return prop.getNumber();
		case PT_VECTOR3:
			return prop.getVector3().mean();
		case PT_STRING: {
			std::string name = obj->property(propname).getString();
			IG_LOG(L_WARNING) << "[TODO] Replacing texture '" << name << "' by average color" << std::endl;
			if (TextureBuffer.count(name)) {
				uint32 id = TextureBuffer.at(name);
				return TextureAverages.at(id).mean();
			} else {
				IG_LOG(L_ERROR) << "No texture '" << name << "' found!" << std::endl;
				return def;
			}
		}
		default:
			IG_LOG(L_WARNING) << "Unknown property type for '" << propname << "'" << std::endl;
			return def;
		}
	} else {
		return def;
	}
}

} // namespace IG