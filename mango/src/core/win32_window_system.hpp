//! \file      win32_window_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_WIN32_WINDOW_SYSTEM_HPP
#define MANGO_WIN32_WINDOW_SYSTEM_HPP

#include <core/window_system_impl.hpp>

namespace mango
{
    //! \brief The \a window_system for the microsoft windows platform.
    class win32_window_system : public window_system_impl
    {
      public:
        win32_window_system();
        ~win32_window_system();
        bool create() override;
        void configure(const window_configuration& configuration) override;
        void swap_buffers() override;
        void update(float dt) override;
        void poll_events() override;
        bool should_close() override;
        void destroy() override;

      private:
        //! \brief The \a window_configuration for the \a win32_window_system.
        //! \details This holds the information that is needed to create a window.
        window_configuration m_window_configuration;
        //! \brief The window handle that is needed to identify the window after creation.
        //! \details This is important, because without it destruction, update and input polling would fail.
        void* m_window_handle;
    };

} // namespace mango

#endif // MANGO_WIN32_WINDOW_SYSTEM_HPP