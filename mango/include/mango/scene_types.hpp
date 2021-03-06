//! \file      scene_types.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_TYPES_HPP
#define MANGO_SCENE_TYPES_HPP

#include <glm/glm.hpp>
#include <mango/types.hpp>

namespace mango
{
    // fwd
    class vertex_array;
    struct material;
    class texture;

    //! \brief An \a entity. Just a integer used as an id.
    using entity = uint32;
    //! \brief Invalid \a entity.
    const entity invalid_entity = 0;
    //! \brief Maximum number of \a entities in mango.
    const entity max_entities = 1000; // Extend if necessary.

    //! \brief Component used to transform anything in the scene.
    struct transform_component
    {
        glm::vec3 position = glm::vec3(0.0f); //!< The local position.
        // TODO Paul: We should really use quaternions.
        glm::vec4 rotation = glm::vec4(0.0f, glm::vec3(0.1f)); //!< The local rotation angle (x) axis (yzw).
        glm::vec3 scale    = glm::vec3(1.0f);                  //!< The local scale.

        glm::mat4 local_transformation_matrix = glm::mat4(1.0f); //!< The local transformation.
        glm::mat4 world_transformation_matrix = glm::mat4(1.0f); //!< The world transformation. If there is no parent this is also the local transformation.
    };

    //! \brief Component used to build a graph like structure. This is necessary for parenting.
    struct node_component
    {
        entity parent_entity = invalid_entity; //!< The parents entity id.
    };

    //! \brief Camera types used in \a camera_components.
    enum class camera_type : uint8
    {
        perspective_camera, //!< Perspective projection. Usually useful for 3D scenes.
        orthographic_camera //!< Orthographic projection. Usually useful for 2D scenes or UI.
    };

    //! \brief Component used to describe a primitive draw call. Used by \a mesh_component.
    struct primitive_component
    {
        shared_ptr<vertex_array> vertex_array_object; //!< The vertex array object of the primitive.
        primitive_topology topology;                  //!< Topology of the primitive data.
        uint32 first;                                 //!< First index.
        uint32 count;                                 //!< Number of elements/vertices.
        index_type type_index;                        //!< The type of the values in the index buffer.
        uint32 instance_count;                        //!< Number of instances. Usually 1.
    };

    //! \brief Component used for materials.
    struct material_component
    {
        shared_ptr<material> component_material; //!< The material holding all properties, textures etc.
    };

    //! \brief Component used for renderable mesh geometry. Used for drawing.
    struct mesh_component
    {
        //! \brief A list of \a primitive_components.
        std::vector<primitive_component> primitives;
        //! \brief A list of \a material_components.
        std::vector<material_component> materials;

        //! \brief Specifies if the mesh has normals.
        bool has_normals;
        //! \brief Specifies if the mesh has tangents.
        bool has_tangents;
    };

    //! \brief Component used for camera entities.
    struct camera_component
    {
        camera_type type; //!< The type of camera projection.

        float z_near;                 //!< Distance of the near plane.
        float z_far;                  //!< Distance of the far plane.
        float vertical_field_of_view; //!< Vertical field of view in radians.
        float aspect;                 //!< Aspect ratio. Width divided by height.
        glm::vec3 up;                 //!< The cameras up vector.
        glm::vec3 target;             //!< The target to look at.
        //! \brief The view matrix of the \a camera_component.
        glm::mat4 view;
        //! \brief The projection matrix of the \a camera_component.
        glm::mat4 projection;
        //! \brief The view projection matrix of the \a camera_component.
        glm::mat4 view_projection;
    };

    //! \brief Structure used for collecting all the camera data of the current active camera.
    struct camera_data
    {
        camera_component* camera_info;  //!< The camera info.
        transform_component* transform; //!< The cameras transform.
    };

    //! \brief Component used for the scene environment.
    //! \details This could be extended from the entities, because there will be only one active environment in the scene normally.
    struct environment_component
    {
        glm::mat3 rotation_scale_matrix = glm::mat3(1.0f); //!< The rotation and scale of the environment.
        shared_ptr<texture> hdr_texture;                   //!< The hdr texture used to build the environment.
    };

    // TODO Paul: This will be reworked when we need the reflection for the components.
    //! \cond NO_COND
    template <typename T>
    struct type_name
    {
        static const char* get()
        {
            return typeid(T).name();
        }
    };

    template <>
    struct type_name<transform_component>
    {
        static const char* get()
        {
            return "transform_component";
        }
    };
    template <>
    struct type_name<node_component>
    {
        static const char* get()
        {
            return "node_component";
        }
    };
    template <>
    struct type_name<primitive_component>
    {
        static const char* get()
        {
            return "primitive_component";
        }
    };
    template <>
    struct type_name<material_component>
    {
        static const char* get()
        {
            return "material_component";
        }
    };
    template <>
    struct type_name<mesh_component>
    {
        static const char* get()
        {
            return "mesh_component";
        }
    };
    template <>
    struct type_name<camera_component>
    {
        static const char* get()
        {
            return "camera_component";
        }
    };
    template <>
    struct type_name<environment_component>
    {
        static const char* get()
        {
            return "environment_component";
        }
    };
    //! \endcond
} // namespace mango

#endif // MANGO_SCENE_TYPES_HPP