//! \file      texture_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <glad/glad.h>
#include <graphics/impl/texture_impl.hpp>

using namespace mango;

texture_impl::texture_impl(const texture_configuration& configuration)
    : m_texture_min_filter(configuration.m_texture_min_filter)
    , m_texture_mag_filter(configuration.m_texture_mag_filter)
    , m_texture_wrap_s(configuration.m_texture_wrap_s)
    , m_texture_wrap_t(configuration.m_texture_wrap_t)
    , m_is_standard_color_space(configuration.m_is_standard_color_space)
    , m_generate_mipmaps(configuration.m_generate_mipmaps)
    , m_is_cubemap(configuration.m_is_cubemap)
{
    g_enum type = GL_TEXTURE_2D;

    if (m_is_cubemap)
        type = GL_TEXTURE_CUBE_MAP;

    glCreateTextures(type, 1, &m_name);
    glTextureParameteri(m_name, GL_TEXTURE_MIN_FILTER, filter_parameter_to_gl(m_texture_min_filter));
    glTextureParameteri(m_name, GL_TEXTURE_MAG_FILTER, filter_parameter_to_gl(m_texture_mag_filter));
    glTextureParameteri(m_name, GL_TEXTURE_WRAP_S, wrap_parameter_to_gl(m_texture_wrap_s));
    glTextureParameteri(m_name, GL_TEXTURE_WRAP_T, wrap_parameter_to_gl(m_texture_wrap_t));

    if (m_is_cubemap)
        glTextureParameteri(m_name, GL_TEXTURE_WRAP_R, wrap_parameter_to_gl(m_texture_wrap_t)); // TODO Paul: Extra parameter!
}

texture_impl::~texture_impl()
{
    if (is_created())
        release();
}

void texture_impl::release()
{
    MANGO_ASSERT(is_created(), "Texture not created!");
    glDeleteTextures(1, &m_name);
    m_name = 0; // This is needed for is_created();
}

void texture_impl::set_data(format internal_format, uint32 width, uint32 height, format pixel_format, format type, const void* data)
{
    MANGO_ASSERT(is_created(), "Texture not created!");
    MANGO_ASSERT(width > 0, "Texture width is invalid!");
    MANGO_ASSERT(height > 0, "Texture height is invalid!");
    // TODO Paul: Evtl. assert that internal format, pixel format and type fit.
    m_width           = width;
    m_height          = height;
    m_format          = pixel_format;
    m_internal_format = internal_format;
    m_component_type  = type;

    g_enum gl_internal_f = static_cast<g_enum>(internal_format);
    g_enum gl_pixel_f    = static_cast<g_enum>(pixel_format);
    g_enum gl_type       = static_cast<g_enum>(type);

    if (!m_is_cubemap)
    {
        glTextureStorage2D(m_name, mipmaps(), gl_internal_f, width, height); // TODO Paul: Mipmap number clamp?
        if (data)
        {
            glTextureSubImage2D(m_name, 0, 0, 0, width, height, gl_pixel_f, gl_type, data);
        }
        if (mipmaps())
        {
            glGenerateTextureMipmap(m_name);
        }
    }
    else
    {
        glTextureStorage2D(m_name, mipmaps(), gl_internal_f, width, height);
        if (data)
        {
            for (uint32 i = 0; i < 6; ++i)
                glTextureSubImage3D(m_name, 0, 0, 0, i, width, height, 1, gl_pixel_f, gl_type, data); // TODO Paul: Is this correct?
        }
        if (mipmaps())
        {
            glGenerateTextureMipmap(m_name);
        }
    }
}

void texture_impl::bind_texture_unit(g_uint unit)
{
    MANGO_ASSERT(is_created(), "Texture not created!");
    glBindTextureUnit(unit, m_name);
}

void texture_impl::unbind()
{
    MANGO_ASSERT(is_created(), "Texture not created!");
    glBindTextureUnit(0, 0);
}
