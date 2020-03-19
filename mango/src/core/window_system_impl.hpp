//! \file      window_system_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_WINDOW_SYSTEM_IMPL_HPP
#define MANGO_WINDOW_SYSTEM_IMPL_HPP

#include <mango/window_system.hpp>

namespace mango
{
    //! \brief The implementation of the \a window_system.
    class window_system_impl : public window_system
    {
      public:
        virtual bool create() = 0;
        virtual void configure(const window_configuration& configuration) = 0;

        //! \brief Swaps the buffers in the \a window_system.
        //! \details The underlying window is double buffered.
        //! The function tells the window to swap these. This should be called after the rendering is finished.
        virtual void swap_buffers() = 0;

        //! \brief Polls events of the \a window_system.
        //! \details The underlying window directly communicates with the os.
        //! The call is necessary to retrieve os events like close events.
        virtual void poll_events() = 0;

        virtual void update(float dt) = 0;
        virtual void destroy()        = 0;

        //! \brief Checks if the \a window_system should close.
        //! \details The \a window_system for example should close, if the window received a close event from the os.
        //! \return True if the window_system should close, else false.
        virtual bool should_close() = 0;
    };

} // namespace mango

#endif // MANGO_WINDOW_SYSTEM_IMPL_HPP