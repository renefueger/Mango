//! \file      texture_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_TEXTURE_IMPL_HPP
#define MANGO_TEXTURE_IMPL_HPP

#include <graphics/texture.hpp>

namespace mango
{
    //! \brief The implementation of the \a texture.
    class texture_impl : public texture
    {
      public:
        //! \brief Constructs the \a texture_impl.
        //! \param[in] configuration The \a texture_configuration.
        texture_impl(const texture_configuration& configuration);
        ~texture_impl();

        inline uint32 get_width() override
        {
            return m_width;
        }

        inline uint32 get_height() override
        {
            return m_height;
        }

        inline uint32 mipmaps() override
        {
            return m_generate_mipmaps;
        }

        inline bool is_in_standard_color_space() override
        {
            return m_is_standard_color_space;
        }

        inline format get_format() override
        {
            return m_format;
        }

        inline format get_internal_format() override
        {
            return m_internal_format;
        }

        inline format component_type() override
        {
            return m_component_type;
        }

        inline texture_parameter min_filter() override
        {
            return m_texture_min_filter;
        }

        inline texture_parameter mag_filter() override
        {
            return m_texture_mag_filter;
        }

        inline texture_parameter wrap_s() override
        {
            return m_texture_wrap_s;
        }

        inline texture_parameter wrap_t() override
        {
            return m_texture_wrap_t;
        }

        inline bool is_cubemap() override
        {
            return m_is_cubemap;
        }

        void set_data(format internal_format, uint32 width, uint32 height, format pixel_format, format type, const void* data) override;
        void bind_texture_unit(g_uint unit) override;
        void unbind() override;
        void release() override;

      private:
        //! \brief The width of the \a texture.
        uint32 m_width;
        //! \brief The height of the \a texture.
        uint32 m_height;
        //! \brief Format information.
        format m_format;
        //! \brief Internal format information.
        format m_internal_format;
        //! \brief The component type.
        format m_component_type;
        //! \brief The filter to use when the texture size gets smaller.
        texture_parameter m_texture_min_filter;
        //! \brief The filter to use when the texture size gets bigger.
        texture_parameter m_texture_mag_filter;
        //! \brief The wrapping procedure in s direction for texture coordinates not in [0, 1].
        texture_parameter m_texture_wrap_s;
        //! \brief The wrapping procedure in t direction for texture coordinates not in [0, 1].
        texture_parameter m_texture_wrap_t;
        //! \brief Specifies if the \a texture should be interpreted as SRGB etc.
        bool m_is_standard_color_space;
        //! \brief Number of mipmap levels.
        uint32 m_generate_mipmaps;
        //! \brief Specifies if the texture is a cubemap.
        bool m_is_cubemap;
    };
} // namespace mango

#endif // MANGO_TEXTURE_IMPL_HPP
