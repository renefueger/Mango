//! \file      scene.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <graphics/buffer.hpp>
#include <graphics/texture.hpp>
#include <graphics/vertex_array.hpp>
#include <mango/scene.hpp>
#include <mango/scene_types.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/resource_system.hpp>

using namespace mango;

static void update_scene_boundaries(glm::mat4& trafo, tinygltf::Model& m, tinygltf::Mesh& mesh, glm::vec3& min, glm::vec3& max);

static void scene_graph_update(scene_component_manager<node_component>& nodes, scene_component_manager<transform_component>& transformations);
static void transformation_update(scene_component_manager<transform_component>& transformations);
static void camera_update(scene_component_manager<camera_component>& cameras, scene_component_manager<transform_component>& transformations);
static void render_meshes(shared_ptr<render_system_impl> rs, scene_component_manager<mesh_component>& meshes, scene_component_manager<transform_component>& transformations);

scene::scene(const string& name)
    : m_nodes()
    , m_transformations()
    , m_meshes()
    , m_cameras()
{
    MANGO_UNUSED(name);
    m_active_camera = invalid_entity;
}

scene::~scene() {}

entity scene::create_empty()
{
    static uint32 id = 1;                                                   // TODO Paul: This should be done in a better way!
    MANGO_ASSERT(id < max_entities, "Reached maximum number of entities!"); // TODO Paul: This assertion is false, because of deleted ones there could be places left.
    entity new_entity = id;
    id++;
    return new_entity;
}

void scene::remove_entity(entity e)
{
    detach(e);
    m_transformations.remove_component_from(e);
    m_meshes.remove_component_from(e);
    m_cameras.remove_component_from(e);
    m_environments.remove_component_from(e);
}

entity scene::create_default_camera()
{
    entity camera_entity      = create_empty();
    auto& camera_component    = m_cameras.create_component_for(camera_entity);
    auto& transform_component = m_transformations.create_component_for(camera_entity);

    // default parameters
    camera_component.type                   = camera_type::perspective_camera;
    camera_component.aspect                 = 16.0f / 9.0f;
    camera_component.z_near                 = 0.1f;
    camera_component.z_far                  = 10.0f;
    camera_component.vertical_field_of_view = glm::radians(45.0f);
    camera_component.up                     = glm::vec3(0.0f, 1.0f, 0.0f);
    camera_component.target                 = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 1.5f);

    transform_component.position = position;

    camera_component.view            = glm::lookAt(position, camera_component.target, camera_component.up);
    camera_component.projection      = glm::perspective(camera_component.vertical_field_of_view, camera_component.aspect, camera_component.z_near, camera_component.z_far);
    camera_component.view_projection = camera_component.projection * camera_component.view;

    // Currently the only camera is the active one.
    m_active_camera = camera_entity;

    return camera_entity;
}

std::vector<entity> scene::create_entities_from_model(const string& path)
{
    std::vector<entity> scene_entities;
    entity scene_root = create_empty();
    auto& transform   = m_transformations.create_component_for(scene_root);
    scene_entities.push_back(scene_root);
    shared_ptr<resource_system> rs = m_shared_context->get_resource_system_internal().lock();
    MANGO_ASSERT(rs, "Resource System is invalid!");
    auto start = path.find_last_of("\\/") + 1;
    auto name = path.substr(start, path.find_last_of(".") - start);
    model_configuration config     = { name };
    const shared_ptr<model> loaded = rs->load_gltf(path, config);
    tinygltf::Model& m             = loaded->gltf_model;

    // load the default scene or the first one.
    m_scene_boundaries.max          = glm::vec3(-3.402823e+38f);
    m_scene_boundaries.min          = glm::vec3(3.402823e+38f);
    MANGO_ASSERT(m.scenes.size() > 0, "No scenes in the gltf model found!");
    int scene_id                 = m.defaultScene > -1 ? m.defaultScene : 0;
    const tinygltf::Scene& scene = m.scenes[scene_id];
    for (uint32 i = 0; i < scene.nodes.size(); ++i)
    {
        entity node = build_model_node(scene_entities, m, m.nodes.at(scene.nodes.at(i)), glm::mat4(1.0));

        attach(node, scene_root);
    }

    // normalize scale
    const glm::vec3 scale = glm::vec3(1.0f / (glm::compMax(m_scene_boundaries.max) - glm::compMin(m_scene_boundaries.min)));
    transform.scale       = scale;

    if (m_active_camera == invalid_entity)
    {
        // We have at least one default camera in each scene and at the moment the first camera is the active one everytime.
        create_default_camera();
    }

    m_cameras.get_component_for_entity(m_active_camera)->target = (m_scene_boundaries.max + m_scene_boundaries.min) * 0.5f * scale;
    m_transformations.get_component_for_entity(m_active_camera)->position.y = (m_scene_boundaries.max.y + m_scene_boundaries.min.y) * 0.75f * scale.y;

    return scene_entities;
}

entity scene::create_environment_from_hdr(const string& path, float rendered_mip_level)
{
    entity environment_entity = create_empty();
    auto& environment         = m_environments.create_component_for(environment_entity);

    // default rotation and scale
    environment.rotation_scale_matrix = glm::mat3(1.0f);

    // load image and texture
    shared_ptr<resource_system> res = m_shared_context->get_resource_system_internal().lock();
    MANGO_ASSERT(res, "Resource System is expired!");

    image_configuration img_config;
    img_config.name                    = path.substr(path.find_last_of("/") + 1, path.find_last_of("."));
    img_config.is_standard_color_space = false;
    img_config.is_hdr                  = true;

    auto hdr_image = res->load_image(path, img_config);

    texture_configuration tex_config;
    tex_config.m_generate_mipmaps        = 1;
    tex_config.m_is_standard_color_space = false;
    tex_config.m_texture_min_filter      = texture_parameter::FILTER_LINEAR;
    tex_config.m_texture_mag_filter      = texture_parameter::FILTER_LINEAR;
    tex_config.m_texture_wrap_s          = texture_parameter::WRAP_CLAMP_TO_EDGE;
    tex_config.m_texture_wrap_t          = texture_parameter::WRAP_CLAMP_TO_EDGE;

    texture_ptr hdr_texture = texture::create(tex_config);

    format f        = format::RGBA;
    format internal = format::RGBA32F;
    format type     = format::FLOAT;

    hdr_texture->set_data(internal, hdr_image->width, hdr_image->height, f, type, hdr_image->data);

    environment.hdr_texture = hdr_texture;

    shared_ptr<render_system_impl> rs = m_shared_context->get_render_system_internal().lock();
    MANGO_ASSERT(rs, "Render System is expired!");
    rs->set_environment_texture(environment.hdr_texture, rendered_mip_level); // TODO Paul: Transformation?

    return environment_entity;
}

void scene::update(float dt)
{
    MANGO_UNUSED(dt);
    transformation_update(m_transformations);
    scene_graph_update(m_nodes, m_transformations);
    camera_update(m_cameras, m_transformations);
}

void scene::render()
{
    shared_ptr<render_system_impl> rs = m_shared_context->get_render_system_internal().lock();
    MANGO_ASSERT(rs, "Render System is expired!");

    render_meshes(rs, m_meshes, m_transformations);
}

void scene::attach(entity child, entity parent)
{
    if (m_nodes.contains(child))
    {
        detach(child);
    }

    m_nodes.create_component_for(child).parent_entity = parent;

    // reorder subtrees if necessary
    if (m_nodes.size() > 1)
    {
        m_nodes.for_each(
            [this](node_component, uint32& index) {
                entity possible_parent = m_nodes.entity_at(index);
                for (size_t j = 0; j < index; ++j)
                {
                    const node_component& possible_child = m_nodes.component_at(j);

                    if (possible_child.parent_entity == possible_parent)
                    {
                        m_nodes.move(index, j);
                        ++index; // TODO Paul: Test if this is correct.
                        break;
                    }
                }
            },
            true);
    }

    node_component& parent_component = *m_nodes.get_component_for_entity(child);

    transform_component* parent_transform = m_transformations.get_component_for_entity(parent);
    if (nullptr == parent_transform)
    {
        // create transform component for parent if non-existent
        parent_transform = &m_transformations.create_component_for(parent);
    }

    transform_component* child_transform = m_transformations.get_component_for_entity(child);
    if (nullptr == child_transform)
    {
        // create transform component for child if non-existent
        child_transform = &m_transformations.create_component_for(child);
    }
}

void scene::detach(entity child)
{
    node_component* parent_component = m_nodes.get_component_for_entity(child);

    if (nullptr == parent_component)
    {
        MANGO_LOG_DEBUG("Entity has no parent!");
        return;
    }

    transform_component* child_transform = m_transformations.get_component_for_entity(child);

    if (nullptr != child_transform)
    {
        // Add transformation from parent before removing the node hierarchy
        child_transform->local_transformation_matrix = child_transform->world_transformation_matrix;
    }

    // We want to remove it without crashing the order. In that way, we don't need to sort it again.
    m_nodes.sort_remove_component_from(child);
}

entity scene::build_model_node(std::vector<entity>& entities, tinygltf::Model& m, tinygltf::Node& n, const glm::mat4& parent_world)
{
    entity node     = create_empty();
    auto& transform = m_transformations.create_component_for(node);
    if (n.matrix.size() == 16)
    {
        glm::mat4 input = glm::make_mat4(n.matrix.data());
        glm::quat orient;
        glm::vec3 s;
        glm::vec4 p;
        glm::decompose(input, transform.scale, orient, transform.position, s, p); // TODO Paul: Use quaternions.
        transform.rotation = glm::vec4(glm::angle(orient), glm::axis(orient));
    }
    else
    {
        if (n.translation.size() == 3)
        {
            transform.position = glm::vec3(n.translation[0], n.translation[1], n.translation[2]);
        }
        if (n.rotation.size() == 4)
        {
            glm::quat orient = glm::quat(n.rotation[3], n.rotation[0], n.rotation[1], n.rotation[2]); // TODO Paul: Use quaternions.
            transform.rotation = glm::vec4(glm::angle(orient), glm::axis(orient));
        }
        if (n.scale.size() == 3)
        {
            transform.scale = glm::vec3(n.scale[0], n.scale[1], n.scale[2]);
        }
    }

    glm::mat4 trafo = glm::translate(glm::mat4(1.0), transform.position);
    trafo           = glm::rotate(trafo, transform.rotation.x, glm::vec3(transform.rotation.y, transform.rotation.z, transform.rotation.w));
    trafo           = glm::scale(trafo, transform.scale);

    trafo = parent_world * trafo;

    if (n.mesh > -1)
    {
        MANGO_ASSERT((uint32)n.mesh < m.meshes.size(), "Invalid gltf mesh!");
        build_model_mesh(node, m, m.meshes.at(n.mesh));
        update_scene_boundaries(trafo, m, m.meshes.at(n.mesh), m_scene_boundaries.min, m_scene_boundaries.max);
    }

    entities.push_back(node);

    // build child nodes.
    for (uint32 i = 0; i < n.children.size(); ++i)
    {
        MANGO_ASSERT((uint32)n.children[i] < m.nodes.size(), "Invalid gltf node!");

        entity child = build_model_node(entities, m, m.nodes.at(n.children.at(i)), trafo);
        attach(child, node);
    }

    return node;
}

void scene::build_model_mesh(entity node, tinygltf::Model& m, tinygltf::Mesh& mesh)
{
    auto& component_mesh = m_meshes.create_component_for(node);

    component_mesh.vertex_array_object = vertex_array::create();

    // Bind vertex buffers later, so we do not bind not used ones and can determine tightly packed attribute offsets later.
    struct vao_buffer_data
    {
        buffer_ptr buf;
    };
    std::map<int, vao_buffer_data> index_to_buffer_data;

    for (size_t i = 0; i < m.bufferViews.size(); ++i)
    {
        const tinygltf::BufferView& buffer_view = m.bufferViews[i];
        if (buffer_view.target == 0)
        {
            MANGO_LOG_WARN("Buffer view target is zero!");
            continue;
        }

        const tinygltf::Buffer& t_buffer = m.buffers[buffer_view.buffer];

        buffer_configuration config;
        config.m_access = buffer_access::NONE;
        config.m_size   = buffer_view.byteLength;
        config.m_target = buffer_view.target == GL_ARRAY_BUFFER ? buffer_target::VERTEX_BUFFER : buffer_target::INDEX_BUFFER;

        const void* buffer_data = static_cast<const void*>(t_buffer.data.data() + buffer_view.byteOffset);
        config.m_data           = buffer_data;
        buffer_ptr buf          = buffer::create(config);

        vao_buffer_data data = { buf };
        index_to_buffer_data.insert({ i, data });
    }

    for (size_t i = 0; i < mesh.primitives.size(); ++i)
    {
        const tinygltf::Primitive& primitive = mesh.primitives[i];

        if (primitive.indices < 0)
        {
            MANGO_LOG_DEBUG("No primitives in this gltf mesh!");
            return;
        }

        const tinygltf::Accessor& index_accessor = m.accessors[primitive.indices];

        primitive_component p;
        p.count          = index_accessor.count;
        p.topology       = static_cast<primitive_topology>(primitive.mode); // cast is okay.
        p.first          = index_accessor.byteOffset;
        p.type_index     = static_cast<index_type>(index_accessor.componentType);
        p.instance_count = 1;

        component_mesh.primitives.push_back(p);

        auto it = index_to_buffer_data.find(index_accessor.bufferView);
        if (it == index_to_buffer_data.end())
            continue; // BAD!
        component_mesh.vertex_array_object->bind_index_buffer(it->second.buf);

        material_component mat;
        mat.material             = std::make_shared<material>();
        mat.material->base_color = glm::vec4(glm::vec3(0.9f), 1.0f);
        mat.material->metallic   = 0.0f;
        mat.material->roughness  = 1.0f;

        load_material(mat, primitive, m);

        component_mesh.materials.push_back(mat);

        uint32 vb_idx = 0;

        for (auto& attrib : primitive.attributes)
        {
            const tinygltf::Accessor& accessor = m.accessors[attrib.second];
            if (accessor.sparse.isSparse)
            {
                MANGO_LOG_ERROR("Models with sparse accessors are currently not supported! Undefined behavior!");
                return;
            }

            format attribute_format = get_attribute_format(static_cast<format>(accessor.componentType), accessor.type % 32); // TODO Paul: Type is not handled properly here.

            int attrib_array = -1;
            if (attrib.first.compare("POSITION") == 0)
                attrib_array = 0;
            if (attrib.first.compare("NORMAL") == 0)
                attrib_array = 1;
            if (attrib.first.compare("TEXCOORD_0") == 0)
                attrib_array = 2;
            if (attrib.first.compare("TANGENT") == 0)
                attrib_array = 3;
            if (attrib_array > -1)
            {
                auto it = index_to_buffer_data.find(accessor.bufferView);
                if (it == index_to_buffer_data.end())
                    continue; // BAD!
                ptr_size stride = accessor.ByteStride(m.bufferViews[accessor.bufferView]);
                MANGO_ASSERT(stride > 0, "Broken gltf model! Attribute stride is {0}!", stride);
                component_mesh.vertex_array_object->bind_vertex_buffer(vb_idx, it->second.buf, accessor.byteOffset, stride);
                component_mesh.vertex_array_object->set_vertex_attribute(attrib_array, vb_idx, attribute_format, 0);
                vb_idx++;
            }
            else
                MANGO_LOG_DEBUG("Vertex attribute array is ignored: {0}!", attrib.first);
        }
    }
}

void scene::load_material(material_component& material, const tinygltf::Primitive& primitive, tinygltf::Model& m)
{
    if (primitive.material < 0)
        return;

    const tinygltf::Material& p_m = m.materials[primitive.material];
    if (!p_m.name.empty())
        MANGO_LOG_DEBUG("Loading material: {0}", p_m.name.c_str());

    auto& pbr = p_m.pbrMetallicRoughness;

    // TODO Paul: Cleanup.

    texture_configuration config;
    config.m_generate_mipmaps        = 1;
    config.m_is_standard_color_space = true;
    config.m_texture_min_filter      = texture_parameter::FILTER_LINEAR_MIPMAP_LINEAR;
    config.m_texture_mag_filter      = texture_parameter::FILTER_LINEAR;
    config.m_texture_wrap_s          = texture_parameter::WRAP_REPEAT;
    config.m_texture_wrap_t          = texture_parameter::WRAP_REPEAT;

    if (pbr.baseColorTexture.index < 0)
    {
        auto col                      = pbr.baseColorFactor;
        material.material->base_color = glm::vec4((float)col[0], (float)col[1], (float)col[2], (float)col[3]);
    }
    else
    {
        // base color
        const tinygltf::Texture& base_col = m.textures.at(pbr.baseColorTexture.index);

        if (base_col.source < 0)
            return;

        const tinygltf::Image& image     = m.images[base_col.source];
        const tinygltf::Sampler& sampler = m.samplers[base_col.sampler];

        if (base_col.sampler >= 0)
        {
            config.m_texture_min_filter = filter_parameter_from_gl(sampler.minFilter);
            config.m_texture_mag_filter = filter_parameter_from_gl(sampler.magFilter);
            config.m_texture_wrap_s     = wrap_parameter_from_gl(sampler.wrapS);
            config.m_texture_wrap_t     = wrap_parameter_from_gl(sampler.wrapT);
        }

        config.m_is_standard_color_space = true;
        config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
        texture_ptr base_color           = texture::create(config);

        format f        = format::RGBA;
        format internal = format::SRGB8_ALPHA8;

        if (image.component == 1)
        {
            f = format::RED;
        }
        else if (image.component == 2)
        {
            f = format::RG;
        }
        else if (image.component == 3)
        {
            f        = format::RGB;
            internal = format::SRGB8;
        }

        format type = format::UNSIGNED_BYTE;
        if (image.bits == 16)
        {
            type = format::UNSIGNED_SHORT;
        }
        else if (image.bits == 32)
        {
            type = format::UNSIGNED_INT;
        }

        base_color->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
        material.material->base_color_texture = base_color;
    }

    // metallic / roughness
    if (pbr.metallicRoughnessTexture.index < 0)
    {
        material.material->metallic  = pbr.metallicFactor;
        material.material->roughness = pbr.roughnessFactor;
    }
    else
    {
        const tinygltf::Texture& o_r_m_t = m.textures.at(pbr.metallicRoughnessTexture.index);

        if (o_r_m_t.source < 0)
            return;

        const tinygltf::Image& image     = m.images[o_r_m_t.source];
        const tinygltf::Sampler& sampler = m.samplers[o_r_m_t.sampler];

        if (o_r_m_t.sampler >= 0)
        {
            config.m_texture_min_filter = filter_parameter_from_gl(sampler.minFilter);
            config.m_texture_mag_filter = filter_parameter_from_gl(sampler.magFilter);
            config.m_texture_wrap_s     = wrap_parameter_from_gl(sampler.wrapS);
            config.m_texture_wrap_t     = wrap_parameter_from_gl(sampler.wrapT);
        }

        config.m_is_standard_color_space = false;
        config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
        texture_ptr o_r_m                = texture::create(config);

        format f        = format::RGBA;
        format internal = format::RGBA8;

        if (image.component == 1)
        {
            f = format::RED;
        }
        else if (image.component == 2)
        {
            f = format::RG;
        }
        else if (image.component == 3)
        {
            f        = format::RGB;
            internal = format::RGB8;
        }

        format type = format::UNSIGNED_BYTE;
        if (image.bits == 16)
        {
            type = format::UNSIGNED_SHORT;
        }
        else if (image.bits == 32)
        {
            type = format::UNSIGNED_INT;
        }

        o_r_m->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
        material.material->roughness_metallic_texture = o_r_m;
    }

    // occlusion
    if (p_m.occlusionTexture.index >= 0)
    {
        if (pbr.metallicRoughnessTexture.index == p_m.occlusionTexture.index)
        {
            // occlusion packed into r channel of the roughness and metallic texture.
            material.material->packed_occlusion = true;
        }
        else
        {
            material.material->packed_occlusion = false;
            const tinygltf::Texture& occ        = m.textures.at(p_m.occlusionTexture.index);
            if (occ.source < 0)
                return;

            const tinygltf::Image& image     = m.images[occ.source];
            const tinygltf::Sampler& sampler = m.samplers[occ.sampler];

            if (occ.sampler >= 0)
            {
                config.m_texture_min_filter = filter_parameter_from_gl(sampler.minFilter);
                config.m_texture_mag_filter = filter_parameter_from_gl(sampler.magFilter);
                config.m_texture_wrap_s     = wrap_parameter_from_gl(sampler.wrapS);
                config.m_texture_wrap_t     = wrap_parameter_from_gl(sampler.wrapT);
            }

            config.m_is_standard_color_space = false;
            config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
            texture_ptr occlusion            = texture::create(config);

            format f        = format::RGBA;
            format internal = format::RGBA8;

            if (image.component == 1)
            {
                f = format::RED;
            }
            else if (image.component == 2)
            {
                f = format::RG;
            }
            else if (image.component == 3)
            {
                f        = format::RGB;
                internal = format::RGB8;
            }

            format type = format::UNSIGNED_BYTE;
            if (image.bits == 16)
            {
                type = format::UNSIGNED_SHORT;
            }
            else if (image.bits == 32)
            {
                type = format::UNSIGNED_INT;
            }

            occlusion->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
            material.material->occlusion_texture = occlusion;
        }
    }

    // normal
    if (p_m.normalTexture.index >= 0)
    {
        const tinygltf::Texture& norm = m.textures.at(p_m.normalTexture.index);

        if (norm.source < 0)
            return;

        const tinygltf::Image& image     = m.images[norm.source];
        const tinygltf::Sampler& sampler = m.samplers[norm.sampler];

        if (norm.sampler >= 0)
        {
            config.m_texture_min_filter = filter_parameter_from_gl(sampler.minFilter);
            config.m_texture_mag_filter = filter_parameter_from_gl(sampler.magFilter);
            config.m_texture_wrap_s     = wrap_parameter_from_gl(sampler.wrapS);
            config.m_texture_wrap_t     = wrap_parameter_from_gl(sampler.wrapT);
        }

        config.m_is_standard_color_space = false;
        config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
        texture_ptr normal_t             = texture::create(config);

        format f        = format::RGBA;
        format internal = format::RGBA8;

        if (image.component == 1)
        {
            f = format::RED;
        }
        else if (image.component == 2)
        {
            f = format::RG;
        }
        else if (image.component == 3)
        {
            f        = format::RGB;
            internal = format::RGB8;
        }

        format type = format::UNSIGNED_BYTE;
        if (image.bits == 16)
        {
            type = format::UNSIGNED_SHORT;
        }
        else if (image.bits == 32)
        {
            type = format::UNSIGNED_INT;
        }

        normal_t->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
        material.material->normal_texture = normal_t;
    }

    // emissive
    if (p_m.emissiveTexture.index < 0)
    {
        auto col                          = p_m.emissiveFactor;
        material.material->emissive_color = glm::vec4((float)col[0], (float)col[1], (float)col[2], (float)col[3]);
        return;
    }
    else
    {
        const tinygltf::Texture& emissive = m.textures.at(p_m.emissiveTexture.index);

        if (emissive.source < 0)
            return;

        const tinygltf::Image& image     = m.images[emissive.source];
        const tinygltf::Sampler& sampler = m.samplers[emissive.sampler];

        if (emissive.sampler >= 0)
        {
            config.m_texture_min_filter = filter_parameter_from_gl(sampler.minFilter);
            config.m_texture_mag_filter = filter_parameter_from_gl(sampler.magFilter);
            config.m_texture_wrap_s     = wrap_parameter_from_gl(sampler.wrapS);
            config.m_texture_wrap_t     = wrap_parameter_from_gl(sampler.wrapT);
        }

        config.m_is_standard_color_space = true;
        config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
        texture_ptr emissive_color       = texture::create(config);

        format f        = format::RGBA;
        format internal = format::SRGB8_ALPHA8;

        if (image.component == 1)
        {
            f = format::RED;
        }
        else if (image.component == 2)
        {
            f = format::RG;
        }
        else if (image.component == 3)
        {
            f        = format::RGB;
            internal = format::SRGB8;
        }

        format type = format::UNSIGNED_BYTE;
        if (image.bits == 16)
        {
            type = format::UNSIGNED_SHORT;
        }
        else if (image.bits == 32)
        {
            type = format::UNSIGNED_INT;
        }

        emissive_color->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
        material.material->emissive_color_texture = emissive_color;
    }
}

static void scene_graph_update(scene_component_manager<node_component>& nodes, scene_component_manager<transform_component>& transformations)
{
    nodes.for_each(
        [&nodes, &transformations](node_component& c, uint32& index) {
            const node_component& parent_component = c;
            entity e                               = nodes.entity_at(index);

            transform_component* child_transform  = transformations.get_component_for_entity(e);
            transform_component* parent_transform = transformations.get_component_for_entity(parent_component.parent_entity);
            if (nullptr != child_transform && nullptr != parent_transform)
            {
                child_transform->world_transformation_matrix = parent_transform->world_transformation_matrix * child_transform->local_transformation_matrix;
            }
        },
        false);
}

static void transformation_update(scene_component_manager<transform_component>& transformations)
{
    transformations.for_each(
        [&transformations](transform_component& c, uint32& index) {
            entity e                      = transformations.entity_at(index);
            c.local_transformation_matrix = glm::translate(glm::mat4(1.0), c.position);
            c.local_transformation_matrix = glm::rotate(c.local_transformation_matrix, c.rotation.x, glm::vec3(c.rotation.y, c.rotation.z, c.rotation.w));
            c.local_transformation_matrix = glm::scale(c.local_transformation_matrix, c.scale);

            c.world_transformation_matrix = c.local_transformation_matrix;
        },
        false);
}

static void camera_update(scene_component_manager<camera_component>& cameras, scene_component_manager<transform_component>& transformations)
{
    cameras.for_each(
        [&cameras, &transformations](camera_component& c, uint32& index) {
            entity e                       = cameras.entity_at(index);
            transform_component* transform = transformations.get_component_for_entity(e);
            if (transform)
            {
                glm::vec3 front = glm::normalize(c.target - transform->position);
                auto right      = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), front)); // TODO Paul: Global up vector?
                c.up            = glm::normalize(glm::cross(front, right));
                c.view          = glm::lookAt(transform->position, c.target, c.up);
                if (c.type == camera_type::perspective_camera)
                {
                    c.projection = glm::perspective(c.vertical_field_of_view, c.aspect, c.z_near, c.z_far);
                }
                else if (c.type == camera_type::orthographic_camera)
                {
                    const float distance = c.z_far - c.z_near;
                    c.projection         = glm::ortho(-c.aspect * distance, c.aspect * distance, -distance, distance);
                }
                c.view_projection = c.projection * c.view;
            }
        },
        false);
}

static void render_meshes(shared_ptr<render_system_impl> rs, scene_component_manager<mesh_component>& meshes, scene_component_manager<transform_component>& transformations)
{
    meshes.for_each(
        [&rs, &meshes, &transformations](mesh_component& c, uint32& index) {
            entity e                       = meshes.entity_at(index);
            transform_component* transform = transformations.get_component_for_entity(e);
            if (transform)
            {
                auto cmdb = rs->get_command_buffer();
                cmdb->bind_vertex_array(c.vertex_array_object);
                rs->set_model_matrix(transform->world_transformation_matrix);

                for (uint32 i = 0; i < c.primitives.size(); ++i)
                {
                    auto m = c.materials[i];
                    auto p = c.primitives[i];
                    rs->draw_mesh(m.material, p.topology, p.first, p.count, p.type_index, p.instance_count);
                }
            }
        },
        false);
}

static void update_scene_boundaries(glm::mat4& trafo, tinygltf::Model& m, tinygltf::Mesh& mesh, glm::vec3& min, glm::vec3& max)
{
    if (mesh.primitives.empty())
        return;

    glm::vec3 min_a;
    glm::vec3 max_a;
    glm::vec3 center;
    glm::vec3 to_center;
    float radius;

    for (size_t j = 0; j < mesh.primitives.size(); ++j)
    {
        const tinygltf::Primitive& primitive = mesh.primitives[j];

        auto attrib = primitive.attributes.find("POSITION");
        if (attrib == primitive.attributes.end())
            continue;

        const tinygltf::Accessor& accessor = m.accessors[attrib->second];

        max_a.x = (float)accessor.maxValues[0];
        max_a.y = (float)accessor.maxValues[1];
        max_a.z = (float)accessor.maxValues[2];
        min_a.x = (float)accessor.minValues[0];
        min_a.y = (float)accessor.minValues[1];
        min_a.z = (float)accessor.minValues[2];

        max_a = glm::vec3(trafo * glm::vec4(max_a, 1.0f));
        min_a = glm::vec3(trafo * glm::vec4(min_a, 1.0f));

        center    = (max_a + min_a) * 0.5f;
        to_center = max_a - center;
        radius    = glm::length(to_center);

        max_a = center + glm::vec3(radius);
        min_a = center - glm::vec3(radius);

        max = glm::max(max, max_a);
        min = glm::min(min, min_a);
    }
}
