//! \file      graphics_state.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_GRAPHICS_STATE
#define MANGO_GRAPHICS_STATE

#include <graphics/graphics_common.hpp>

namespace mango
{
    //! \brief Holds information about the current state of a graphics pipeline.
    //! \details This is mostly used to avoid unnecessary calls to the gpu.
    //! The calls do only change values in this state; there is nothing changed in the real graphics state.
    //! All functions return true, if the values in the current state were changed, else false.
    //! This is used to check if a real call is required.
    struct graphics_state
    {
      public:
        graphics_state();
        ~graphics_state() = default;

        //! \brief Sets the viewport size.
        //! \param[in] x The x position of the viewport.
        //! \param[in] y The y position of the viewport.
        //! \param[in] width The width of the viewport.
        //! \param[in] height The height of the viewport.
        //! \return True if state changed, else false.
        bool set_viewport(uint32 x, uint32 y, uint32 width, uint32 height);

        //! \brief Enables or disables the depth test.
        //! \param[in] enabled True if the depth test should be enabled, else false.
        //! \return True if state changed, else false.
        bool set_depth_test(bool enabled);

        //! \brief Sets the \a compare_operation for depth testing.
        //! \param[in] op The \a compare_operation to use for depth testing.
        //! \return True if state changed, else false.
        bool set_depth_func(compare_operation op);

        //! \brief Sets the \a polygon_mode as well as the \a polygon_faces used for drawing.
        //! \param[in] face The \a polygon_faces to draw.
        //! \param[in] mode The \a polygon_mode used for drawing.
        //! \return True if state changed, else false.
        bool set_polygon_mode(polygon_face face, polygon_mode mode);

        //! \brief Binds a \a vertex_array for drawing.
        //! \param[in] vertex_array A pointer to the \a vertex_array to bind.
        //! \return True if state changed, else false.
        bool bind_vertex_array(vertex_array_ptr vertex_array);

        //! \brief Binds a \a shader_program for drawing.
        //! \param[in] shader_program A pointer to the \a shader_program to bind.
        //! \return True if state changed, else false.
        bool bind_shader_program(shader_program_ptr shader_program);

        //! \brief Binds a non buffered uniform.
        //! \details This gets reset after every draw call.
        //! \return True if state changed, else false.
        bool bind_single_uniform();

        //! \brief Binds an \a uniform \a buffer for drawing.
        //! \param[in] index The \a uniform \a buffer index to bind the \a buffer to.
        //! \param[in] uniform_buffer The \a uniform \a buffer to bind.
        //! \return True if state changed, else false.
        bool bind_uniform_buffer(g_uint index, buffer_ptr uniform_buffer);

        //! \brief Binds a \a texture for drawing.
        //! \param[in] binding The binding location to bind the \a texture too.
        //! \param[in] name The name of the \a texture.
        //! \return True if state changed, else false.
        bool bind_texture(uint32 binding, uint32 name);

        //! \brief Binds a \a framebuffer for drawing.
        //! \param[in] framebuffer The pointer to the \a framebuffer to bind.
        //! \return True if state changed, else false.
        bool bind_framebuffer(framebuffer_ptr framebuffer);

        //! \brief Enables or disables face culling.
        //! \param[in] enabled True if face culling should be enabled, else false.
        //! \return True if the face culling state changed, else false.
        bool set_face_culling(bool enabled);

        //! \brief Sets the \a polygon_face for face culling.
        //! \param[in] face The \a polygon_face to use for face culling.
        //! \return True if state changed, else false.
        bool set_cull_face(polygon_face face);

        //! \brief Enables or disables blending.
        //! \param[in] enabled True if blending should be enabled, else false.
        //! \return True if the the blending state changed, else false.
        bool set_blending(bool enabled);

        //! \brief Sets the \a blend_factors for blending.
        //! \param[in] source The \a blend_factor influencing the source value.
        //! \param[in] destination The \a blend_factor influencing the destination value.
        //! \return True if state changed, else false.
        bool set_blend_factors(blend_factor source, blend_factor destination);

        //! \brief The maximum number of texture bindings (not really, just supported by the state).
        const static uint32 max_texture_bindings = 16; // TODO Paul: We should really define these things somewhere else. And query from OpenGL.
        //! \brief Structure to cache the state of the graphics pipeline.
        struct internal_state
        {
            shader_program_ptr shader_program; //!< Cached shader program.
            framebuffer_ptr framebuffer;       //!< Cached framebuffer.
            vertex_array_ptr vertex_array;     //!< Cached vertex array.

            std::array<uint32, max_texture_bindings> m_active_texture_bindings; //!< Bindings from binding points to texture names.

            struct
            {
                uint32 x;      //!< Viewport x position.
                uint32 y;      //!< Viewport y position.
                uint32 width;  //!< Viewport width.
                uint32 height; //!< Viewport height.
            } viewport;        //!< Cached viewport.

            struct
            {
                polygon_face face; //!< Polygon mode face.
                polygon_mode mode; //!< Polygon mode.
            } poly_mode;           //!< Cached polygon mode.

            struct
            {
                bool enabled;                 //!< Enabled or disabled.
                compare_operation depth_func; //!< Compare operation.
            } depth_test;                     //!< Cached depth test.

            struct
            {
                bool enabled;      //!< Enabled or disabled.
                polygon_face face; //!< Polygon face.
            } face_culling;        //!< Cached face cull state.

            struct
            {
                bool enabled;      //!< Enabled or disabled.
                blend_factor src;  //!< Source blend factor.
                blend_factor dest; //!< Destination blend factor.
            } blending;            //!< Cached blend state.
        } m_internal_state;        //!< The internal state.
    };
} // namespace mango

#endif // MANGO_GRAPHICS_STATE