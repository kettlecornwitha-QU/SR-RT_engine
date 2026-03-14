#include "scene/model_loader.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "geometry/triangle.h"
#include "materials/emissive.h"
#include "materials/lambertian.h"
#include "materials/textured_lambertian.h"
#include "render/texture.h"

#if __has_include("tiny_obj_loader.h")
#define HAVE_TINYOBJ 1
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#endif

#if __has_include("tiny_gltf.h")
#if __has_include("nlohmann/json.hpp")
#define HAVE_TINYGLTF 1
#define TINYGLTF_NO_INCLUDE_JSON
#include "nlohmann/json.hpp"
#elif __has_include("json.hpp")
#define HAVE_TINYGLTF 1
#define TINYGLTF_NO_INCLUDE_JSON
#include "json.hpp"
#endif
#endif

#if HAVE_TINYGLTF
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"
#endif

namespace {

namespace fs = std::filesystem;

#if HAVE_TINYOBJ
double max_component(const Vec3& v) {
    return std::max(v.x, std::max(v.y, v.z));
}
#endif

template <typename T, typename... Args>
T* add_material(SceneBuild& s, Args&&... args) {
    auto mat = std::make_shared<T>(std::forward<Args>(args)...);
    T* raw = mat.get();
    s.materials.push_back(mat);
    return raw;
}

#if HAVE_TINYOBJ
Vec3 read_obj_position(const tinyobj::attrib_t& attrib,
                       int vertex_index)
{
    size_t base = static_cast<size_t>(3 * vertex_index);
    return {
        static_cast<double>(attrib.vertices[base + 0]),
        static_cast<double>(attrib.vertices[base + 1]),
        static_cast<double>(attrib.vertices[base + 2])
    };
}

Vec3 read_obj_normal(const tinyobj::attrib_t& attrib,
                     int normal_index)
{
    size_t base = static_cast<size_t>(3 * normal_index);
    return {
        static_cast<double>(attrib.normals[base + 0]),
        static_cast<double>(attrib.normals[base + 1]),
        static_cast<double>(attrib.normals[base + 2])
    };
}

Vec3 read_obj_uv(const tinyobj::attrib_t& attrib,
                 int texcoord_index)
{
    size_t base = static_cast<size_t>(2 * texcoord_index);
    return {
        static_cast<double>(attrib.texcoords[base + 0]),
        static_cast<double>(attrib.texcoords[base + 1]),
        0.0
    };
}

const Material* create_obj_material(SceneBuild& s,
                                    const tinyobj::material_t& m,
                                    const std::string& base_dir)
{
    Vec3 kd = {
        static_cast<double>(m.diffuse[0]),
        static_cast<double>(m.diffuse[1]),
        static_cast<double>(m.diffuse[2])
    };
    Vec3 ke = {
        static_cast<double>(m.emission[0]),
        static_cast<double>(m.emission[1]),
        static_cast<double>(m.emission[2])
    };

    if (max_component(ke) > 0.0)
        return add_material<Emissive>(s, ke);

    if (!m.diffuse_texname.empty()) {
        fs::path tex_path = fs::path(base_dir) / fs::path(m.diffuse_texname);
        std::string tex_msg;
        std::shared_ptr<ImageTexture> tex = ImageTexture::load(tex_path.string(), tex_msg);
        if (tex && tex->valid()) {
            return add_material<TexturedLambertian>(s, tex, kd);
        }
        std::cout << "Texture load warning: " << tex_path.string() << " (" << tex_msg << ")\n";
    }

    return add_material<Lambertian>(s, kd);
}
#endif

bool load_obj_tinyobj(const std::string& path,
                      SceneBuild& s,
                      int& tri_count)
{
#if HAVE_TINYOBJ
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    config.mtl_search_path = fs::path(path).parent_path().string();

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path, config)) {
        std::string err = reader.Error();
        if (!err.empty())
            std::cerr << "OBJ parse error: " << err << "\n";
        return false;
    }

    std::string warn = reader.Warning();
    if (!warn.empty())
        std::cout << "OBJ parse warning: " << warn << "\n";

    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();
    const std::vector<tinyobj::material_t>& materials = reader.GetMaterials();

    const Material* default_material = add_material<Lambertian>(s, Vec3{0.7, 0.7, 0.7});
    std::vector<const Material*> material_lut(materials.size(), default_material);

    const std::string base_dir = fs::path(path).parent_path().string();
    for (size_t i = 0; i < materials.size(); ++i)
        material_lut[i] = create_obj_material(s, materials[i], base_dir);

    tri_count = 0;
    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f];
            if (fv < 3) {
                index_offset += static_cast<size_t>(fv);
                continue;
            }

            int material_id = -1;
            if (f < shape.mesh.material_ids.size())
                material_id = shape.mesh.material_ids[f];
            const Material* material =
                (material_id >= 0 && static_cast<size_t>(material_id) < material_lut.size())
                ? material_lut[static_cast<size_t>(material_id)]
                : default_material;

            for (int k = 1; k + 1 < fv; ++k) {
                tinyobj::index_t idx0 = shape.mesh.indices[index_offset + 0];
                tinyobj::index_t idx1 = shape.mesh.indices[index_offset + static_cast<size_t>(k)];
                tinyobj::index_t idx2 = shape.mesh.indices[index_offset + static_cast<size_t>(k + 1)];

                if (idx0.vertex_index < 0 || idx1.vertex_index < 0 || idx2.vertex_index < 0)
                    continue;

                Vec3 v0 = read_obj_position(attrib, idx0.vertex_index);
                Vec3 v1 = read_obj_position(attrib, idx1.vertex_index);
                Vec3 v2 = read_obj_position(attrib, idx2.vertex_index);

                bool has_uv =
                    idx0.texcoord_index >= 0 &&
                    idx1.texcoord_index >= 0 &&
                    idx2.texcoord_index >= 0 &&
                    (2 * idx0.texcoord_index + 1) < static_cast<int>(attrib.texcoords.size()) &&
                    (2 * idx1.texcoord_index + 1) < static_cast<int>(attrib.texcoords.size()) &&
                    (2 * idx2.texcoord_index + 1) < static_cast<int>(attrib.texcoords.size());

                bool has_normals =
                    idx0.normal_index >= 0 &&
                    idx1.normal_index >= 0 &&
                    idx2.normal_index >= 0 &&
                    (3 * idx0.normal_index + 2) < static_cast<int>(attrib.normals.size()) &&
                    (3 * idx1.normal_index + 2) < static_cast<int>(attrib.normals.size()) &&
                    (3 * idx2.normal_index + 2) < static_cast<int>(attrib.normals.size());

                Vec3 uv0 = has_uv ? read_obj_uv(attrib, idx0.texcoord_index) : Vec3{0, 0, 0};
                Vec3 uv1 = has_uv ? read_obj_uv(attrib, idx1.texcoord_index) : Vec3{0, 0, 0};
                Vec3 uv2 = has_uv ? read_obj_uv(attrib, idx2.texcoord_index) : Vec3{0, 0, 0};
                Vec3 n0 = has_normals ? read_obj_normal(attrib, idx0.normal_index) : Vec3{0, 1, 0};
                Vec3 n1 = has_normals ? read_obj_normal(attrib, idx1.normal_index) : Vec3{0, 1, 0};
                Vec3 n2 = has_normals ? read_obj_normal(attrib, idx2.normal_index) : Vec3{0, 1, 0};

                s.world.add(std::make_shared<Triangle>(
                    v0, v1, v2, material,
                    uv0, uv1, uv2,
                    n0, n1, n2,
                    has_uv, has_normals));
                tri_count++;
            }

            index_offset += static_cast<size_t>(fv);
        }
    }

    return tri_count > 0;
#else
    (void)path;
    (void)s;
    tri_count = 0;
    return false;
#endif
}

#if HAVE_TINYGLTF
size_t gltf_component_size(int component_type) {
    switch (component_type) {
        case TINYGLTF_COMPONENT_TYPE_BYTE: return 1;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return 1;
        case TINYGLTF_COMPONENT_TYPE_SHORT: return 2;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return 2;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return 4;
        case TINYGLTF_COMPONENT_TYPE_FLOAT: return 4;
        default: return 0;
    }
}

int gltf_num_components(int accessor_type) {
    switch (accessor_type) {
        case TINYGLTF_TYPE_SCALAR: return 1;
        case TINYGLTF_TYPE_VEC2: return 2;
        case TINYGLTF_TYPE_VEC3: return 3;
        case TINYGLTF_TYPE_VEC4: return 4;
        default: return 0;
    }
}

double gltf_read_component(const unsigned char* ptr, int component_type) {
    switch (component_type) {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            return static_cast<double>(*reinterpret_cast<const int8_t*>(ptr));
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return static_cast<double>(*reinterpret_cast<const uint8_t*>(ptr));
        case TINYGLTF_COMPONENT_TYPE_SHORT:
            return static_cast<double>(*reinterpret_cast<const int16_t*>(ptr));
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return static_cast<double>(*reinterpret_cast<const uint16_t*>(ptr));
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            return static_cast<double>(*reinterpret_cast<const uint32_t*>(ptr));
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return static_cast<double>(*reinterpret_cast<const float*>(ptr));
        default:
            return 0.0;
    }
}

double gltf_read_component_normalized(const unsigned char* ptr, int component_type) {
    switch (component_type) {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            return std::max(-1.0, gltf_read_component(ptr, component_type) / 127.0);
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return gltf_read_component(ptr, component_type) / 255.0;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
            return std::max(-1.0, gltf_read_component(ptr, component_type) / 32767.0);
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return gltf_read_component(ptr, component_type) / 65535.0;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            return gltf_read_component(ptr, component_type) / 4294967295.0;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return gltf_read_component(ptr, component_type);
        default:
            return 0.0;
    }
}

bool gltf_read_accessor_values(const tinygltf::Model& model,
                               const tinygltf::Accessor& accessor,
                               size_t index,
                               std::array<double, 4>& out)
{
    if (accessor.bufferView < 0 ||
        accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
        return false;
    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
    if (view.buffer < 0 ||
        view.buffer >= static_cast<int>(model.buffers.size()))
        return false;
    const tinygltf::Buffer& buffer = model.buffers[view.buffer];

    int comps = gltf_num_components(accessor.type);
    size_t comp_size = gltf_component_size(accessor.componentType);
    if (comps <= 0 || comp_size == 0)
        return false;

    size_t stride = accessor.ByteStride(view);
    if (stride == 0)
        stride = static_cast<size_t>(comps) * comp_size;

    size_t offset = static_cast<size_t>(view.byteOffset) +
                    static_cast<size_t>(accessor.byteOffset) +
                    index * stride;
    if (offset + static_cast<size_t>(comps) * comp_size > buffer.data.size())
        return false;

    out = {0.0, 0.0, 0.0, 0.0};
    for (int c = 0; c < comps; ++c) {
        const unsigned char* p = buffer.data.data() + offset + static_cast<size_t>(c) * comp_size;
        out[static_cast<size_t>(c)] = accessor.normalized
            ? gltf_read_component_normalized(p, accessor.componentType)
            : gltf_read_component(p, accessor.componentType);
    }
    return true;
}

bool gltf_read_index(const tinygltf::Model& model,
                     const tinygltf::Accessor& accessor,
                     size_t index,
                     unsigned int& out)
{
    std::array<double, 4> tmp{};
    if (!gltf_read_accessor_values(model, accessor, index, tmp))
        return false;
    out = static_cast<unsigned int>(tmp[0]);
    return true;
}

const Material* create_gltf_material(SceneBuild& s,
                                     const tinygltf::Material& m,
                                     const std::string& base_dir,
                                     const tinygltf::Model& model,
                                     std::map<std::string, std::shared_ptr<ImageTexture>>& texture_cache)
{
    Vec3 base_color = {1, 1, 1};
    if (m.pbrMetallicRoughness.baseColorFactor.size() == 4) {
        base_color = {
            m.pbrMetallicRoughness.baseColorFactor[0],
            m.pbrMetallicRoughness.baseColorFactor[1],
            m.pbrMetallicRoughness.baseColorFactor[2]
        };
    }

    if (m.emissiveFactor.size() == 3) {
        Vec3 e = {m.emissiveFactor[0], m.emissiveFactor[1], m.emissiveFactor[2]};
        if (max_component(e) > 0.0)
            return add_material<Emissive>(s, e);
    }

    int tex_index = m.pbrMetallicRoughness.baseColorTexture.index;
    if (tex_index >= 0 &&
        tex_index < static_cast<int>(model.textures.size()))
    {
        int source = model.textures[static_cast<size_t>(tex_index)].source;
        if (source >= 0 && source < static_cast<int>(model.images.size())) {
            const tinygltf::Image& img = model.images[static_cast<size_t>(source)];
            if (!img.uri.empty()) {
                fs::path tex_path = fs::path(base_dir) / fs::path(img.uri);
                std::string tex_path_str = tex_path.string();
                auto cached = texture_cache.find(tex_path_str);
                if (cached != texture_cache.end() && cached->second && cached->second->valid())
                    return add_material<TexturedLambertian>(s, cached->second, base_color);

                std::string tex_msg;
                std::shared_ptr<ImageTexture> tex = ImageTexture::load(tex_path_str, tex_msg);
                if (tex && tex->valid()) {
                    texture_cache[tex_path_str] = tex;
                    return add_material<TexturedLambertian>(s, tex, base_color);
                }
                std::cout << "Texture load warning: " << tex_path_str
                          << " (" << tex_msg << ")\n";
            }
        }
    }

    return add_material<Lambertian>(s, base_color);
}

bool load_gltf_tinygltf(const std::string& path,
                        SceneBuild& s,
                        int& tri_count,
                        int triangle_step)
{
    tinygltf::TinyGLTF loader;
    loader.SetImageLoader(
        [](tinygltf::Image* image,
           int, std::string*, std::string*, int, int,
           const unsigned char*, int, void*) -> bool {
            if (image) {
                image->width = 0;
                image->height = 0;
                image->component = 0;
                image->bits = 0;
                image->pixel_type = 0;
                image->image.clear();
            }
            return true;
        },
        nullptr);
    tinygltf::Model model;
    std::string err;
    std::string warn;

    bool ok = false;
    std::string lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".glb")
        ok = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    else
        ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);

    if (!warn.empty())
        std::cout << "glTF warning: " << warn << "\n";
    if (!ok) {
        if (!err.empty())
            std::cerr << "glTF error: " << err << "\n";
        return false;
    }

    const Material* default_material = add_material<Lambertian>(s, Vec3{0.7, 0.7, 0.7});
    std::vector<const Material*> material_lut(model.materials.size(), default_material);
    std::map<std::string, std::shared_ptr<ImageTexture>> texture_cache;
    const std::string base_dir = fs::path(path).parent_path().string();
    for (size_t i = 0; i < model.materials.size(); ++i)
        material_lut[i] = create_gltf_material(s,
                                               model.materials[i],
                                               base_dir,
                                               model,
                                               texture_cache);

    tri_count = 0;
    triangle_step = std::max(1, triangle_step);
    for (const auto& mesh : model.meshes) {
        for (const auto& prim : mesh.primitives) {
            auto pos_it = prim.attributes.find("POSITION");
            if (pos_it == prim.attributes.end())
                continue;
            int pos_accessor_idx = pos_it->second;
            if (pos_accessor_idx < 0 || pos_accessor_idx >= static_cast<int>(model.accessors.size()))
                continue;
            const tinygltf::Accessor& pos_accessor = model.accessors[static_cast<size_t>(pos_accessor_idx)];
            size_t vertex_count = static_cast<size_t>(pos_accessor.count);
            if (vertex_count == 0)
                continue;

            std::vector<Vec3> positions(vertex_count, Vec3{0, 0, 0});
            std::vector<Vec3> normals(vertex_count, Vec3{0, 1, 0});
            std::vector<Vec3> uvs(vertex_count, Vec3{0, 0, 0});
            bool has_normals = false;
            bool has_uv = false;

            for (size_t i = 0; i < vertex_count; ++i) {
                std::array<double, 4> p{};
                if (!gltf_read_accessor_values(model, pos_accessor, i, p))
                    break;
                positions[i] = {p[0], p[1], p[2]};
            }

            auto n_it = prim.attributes.find("NORMAL");
            if (n_it != prim.attributes.end()) {
                int n_accessor_idx = n_it->second;
                if (n_accessor_idx >= 0 && n_accessor_idx < static_cast<int>(model.accessors.size())) {
                    const tinygltf::Accessor& n_accessor = model.accessors[static_cast<size_t>(n_accessor_idx)];
                    if (static_cast<size_t>(n_accessor.count) >= vertex_count) {
                        has_normals = true;
                        for (size_t i = 0; i < vertex_count; ++i) {
                            std::array<double, 4> n{};
                            if (!gltf_read_accessor_values(model, n_accessor, i, n)) {
                                has_normals = false;
                                break;
                            }
                            normals[i] = Vec3{n[0], n[1], n[2]}.normalize();
                        }
                    }
                }
            }

            auto uv_it = prim.attributes.find("TEXCOORD_0");
            if (uv_it != prim.attributes.end()) {
                int uv_accessor_idx = uv_it->second;
                if (uv_accessor_idx >= 0 && uv_accessor_idx < static_cast<int>(model.accessors.size())) {
                    const tinygltf::Accessor& uv_accessor = model.accessors[static_cast<size_t>(uv_accessor_idx)];
                    if (static_cast<size_t>(uv_accessor.count) >= vertex_count) {
                        has_uv = true;
                        for (size_t i = 0; i < vertex_count; ++i) {
                            std::array<double, 4> uv{};
                            if (!gltf_read_accessor_values(model, uv_accessor, i, uv)) {
                                has_uv = false;
                                break;
                            }
                            uvs[i] = {uv[0], uv[1], 0.0};
                        }
                    }
                }
            }

            std::vector<unsigned int> indices;
            if (prim.indices >= 0 &&
                prim.indices < static_cast<int>(model.accessors.size()))
            {
                const tinygltf::Accessor& idx_accessor = model.accessors[static_cast<size_t>(prim.indices)];
                indices.resize(static_cast<size_t>(idx_accessor.count));
                for (size_t i = 0; i < indices.size(); ++i) {
                    if (!gltf_read_index(model, idx_accessor, i, indices[i])) {
                        indices.clear();
                        break;
                    }
                }
            } else {
                indices.resize(vertex_count);
                for (size_t i = 0; i < vertex_count; ++i)
                    indices[i] = static_cast<unsigned int>(i);
            }

            if (indices.size() < 3)
                continue;

            const Material* material = default_material;
            if (prim.material >= 0 &&
                static_cast<size_t>(prim.material) < material_lut.size())
                material = material_lut[static_cast<size_t>(prim.material)];

            for (size_t i = 0; i + 2 < indices.size(); i += static_cast<size_t>(3 * triangle_step)) {
                unsigned int i0 = indices[i + 0];
                unsigned int i1 = indices[i + 1];
                unsigned int i2 = indices[i + 2];
                if (i0 >= positions.size() || i1 >= positions.size() || i2 >= positions.size())
                    continue;

                s.world.add(std::make_shared<Triangle>(
                    positions[i0], positions[i1], positions[i2], material,
                    uvs[i0], uvs[i1], uvs[i2],
                    normals[i0], normals[i1], normals[i2],
                    has_uv, has_normals));
                tri_count++;
            }
        }
    }

    if (tri_count > 0) {
        std::cout << "Loaded " << tri_count << " glTF triangles from " << path;
        if (triangle_step > 1)
            std::cout << " (triangle step " << triangle_step << ")";
        std::cout << "\n";
        return true;
    }
    return false;
}
#else
bool load_gltf_tinygltf(const std::string& path,
                        SceneBuild& s,
                        int& tri_count,
                        int triangle_step)
{
    (void)path;
    (void)s;
    (void)triangle_step;
    tri_count = 0;
    return false;
}
#endif

int parse_obj_vertex_index(const std::string& token, int vertex_count) {
    size_t slash = token.find('/');
    std::string index_str = token.substr(0, slash);
    if (index_str.empty())
        return -1;

    int idx = std::stoi(index_str);
    if (idx > 0)
        return idx - 1;
    if (idx < 0)
        return vertex_count + idx;
    return -1;
}

int load_obj_triangles_fallback(const std::string& path,
                                const Material* material,
                                Scene& world)
{
    std::ifstream in(path);
    if (!in)
        return 0;

    std::vector<Vec3> vertices;
    int tri_count = 0;
    std::string line;

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            double x, y, z;
            if (iss >> x >> y >> z)
                vertices.push_back({x, y, z});
        } else if (type == "f") {
            std::vector<int> face_indices;
            std::string token;
            while (iss >> token) {
                int idx = parse_obj_vertex_index(token, static_cast<int>(vertices.size()));
                if (idx >= 0 && idx < static_cast<int>(vertices.size()))
                    face_indices.push_back(idx);
            }

            if (face_indices.size() < 3)
                continue;

            for (size_t k = 1; k + 1 < face_indices.size(); ++k) {
                const Vec3& a = vertices[face_indices[0]];
                const Vec3& b = vertices[face_indices[k]];
                const Vec3& c = vertices[face_indices[k + 1]];
                world.add(std::make_shared<Triangle>(a, b, c, material));
                tri_count++;
            }
        }
    }

    return tri_count;
}

} // namespace

bool scene_file_exists(const std::string& path) {
    std::error_code ec;
    return fs::exists(fs::path(path), ec);
}

std::string find_existing_scene_path(const std::vector<std::string>& candidates) {
    for (const auto& c : candidates) {
        if (scene_file_exists(c))
            return c;
    }
    return "";
}

bool load_model_scene(const std::string& path,
                      SceneBuild& s,
                      const Material* fallback,
                      int gltf_triangle_step)
{
    std::string lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if ((lower.size() >= 5 && lower.substr(lower.size() - 5) == ".gltf") ||
        (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".glb"))
    {
        int gltf_triangles = 0;
        if (load_gltf_tinygltf(path, s, gltf_triangles, gltf_triangle_step))
            return true;
#if !HAVE_TINYGLTF
        std::cout << "glTF loading unavailable. Install tiny_gltf.h plus JSON headers: include/nlohmann/json.hpp or include/json.hpp.\n";
#endif
        return false;
    }

    int tiny_triangles = 0;
    if (load_obj_tinyobj(path, s, tiny_triangles)) {
        std::cout << "Loaded " << tiny_triangles << " OBJ triangles via tinyobjloader from " << path << "\n";
        return true;
    }

    int fallback_triangles = load_obj_triangles_fallback(path, fallback, s.world);
    if (fallback_triangles > 0) {
#if !HAVE_TINYOBJ
        std::cout << "tiny_obj_loader.h not found; loaded OBJ geometry only (no MTL/UV/textures).\n";
#endif
        std::cout << "Loaded " << fallback_triangles << " OBJ triangles (fallback) from " << path << "\n";
        return true;
    }

    return false;
}

