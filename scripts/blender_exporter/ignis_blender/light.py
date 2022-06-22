import mathutils
import math
from .utils import *
from .node import export_node

# Make sure the Z axis is handled as up, instead of Y
ENVIRONMENT_MAP_TRANSFORM = [0, 1, 0, 0, 0, 1, 1, 0, 0]


def export_background(result, out_dir, scene):
    if "Background" not in scene.world.node_tree.nodes:
        return

    tree = scene.world.node_tree.nodes["Background"]

    if tree.type == "BACKGROUND":
        strength = export_node(result, tree.inputs["Strength"], out_dir)
        radiance = export_node(result, tree.inputs["Color"], out_dir)

        # Check if there is any emission (if we can detect it)
        has_emission = try_extract_node_value(strength, default=1) > 0
        if not has_emission:
            return
        
        has_emission = try_extract_node_value(radiance, default=1) > 0
        if not has_emission:
            return

        try:
            has_emission = radiance[0] > 0 or radiance[1] > 0 or radiance[2] > 0
        except Exception:
            pass

        if not has_emission:
            return
        
        result["lights"].append(
            {"type": "env", "name": "__scene_world", "radiance": radiance, "scale": strength, "transform": ENVIRONMENT_MAP_TRANSFORM})


def export_light(result, inst):
    light = inst.object
    l = light.data
    power = [l.color[0] * l.energy, l.color[1]
             * l.energy, l.color[2] * l.energy]
    position = inst.matrix_world @ mathutils.Vector((0, 0, 0, 1))
    direction = inst.matrix_world @ mathutils.Vector((0, 0, 1, 0))

    if l.type == "POINT":
        result["lights"].append(
            {"type": "point", "name": light.name, "position": map_vector(
                position), "intensity": map_rgb(power)}
        )
    elif l.type == "SPOT":
        result["lights"].append(
            {"type": "spot", "name": light.name, "position": map_vector(position), "direction": map_vector(direction.normalized(
            )), "intensity": map_rgb(power), "cutoff": math.degrees(l.spot_size/2), "falloff": math.degrees(l.spot_size/2)*(1 - l.spot_blend)}
        )
    elif l.type == "SUN":
        result["lights"].append(
            {"type": "direction", "name": light.name, "direction": map_vector(
                -direction.normalized()), "irradiance": map_rgb(power)}
        )
    elif l.type == "AREA":
        size_x = l.size
        if l.shape == 'SQUARE':
            size_y = l.size
        else:
            size_y = l.size_y

        if l.shape == "DISK":
            result["shapes"].append(
                {"type": "disk", "name": light.name +
                    "-shape", "radius": size_x/2, "flip_normals": True}
            )
        elif l.shape == "ELLIPSE":
            # Approximate by non-uniformly scaling the uniform disk
            size_y = l.size_y
            result["shapes"].append(
                {"type": "disk", "name": light.name +
                    "-shape", "radius": 1, "flip_normals": True, "transform": [size_x/2, 0, 0, 0, size_y/2, 0, 0, 0, 1]}
            )
        else:
            result["shapes"].append(
                {"type": "rectangle", "name": light.name +
                    "-shape", "width": size_x, "height": size_y, "flip_normals": True}
            )
        result["bsdfs"].append(
            {"type": "diffuse", "name": light.name +
                "-bsdf", "reflectance": [0, 0, 0]}
        )
        result["entities"].append(
            {"name": light.name, "shape": light.name +
                "-shape", "bsdf": light.name+"-bsdf", "transform": flat_matrix(inst.matrix_world)}
        )
        result["lights"].append(
            {"type": "area", "name": light.name, "entity": light.name,
                "radiance": map_rgb(power)}
        )
