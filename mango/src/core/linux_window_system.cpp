//! \file      linux_window_system.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#define GLFW_INCLUDE_NONE // for glad
#include <GLFW/glfw3.h>
#include <core/linux_window_system.hpp>
#include <graphics/command_buffer.hpp>
#include <mango/assert.hpp>
#include <mango/scene.hpp>
#include <rendering/render_system_impl.hpp>

using namespace mango;

linux_window_system::linux_window_system(const shared_ptr<context_impl>& context)
    : m_window_configuration()
    , m_window_handle(nullptr)
{
    m_window_user_data.shared_context = context;
}

linux_window_system::~linux_window_system() {}

bool linux_window_system::create()
{
    if (!glfwInit())
    {
        MANGO_LOG_ERROR("Initilization of glfw failed! No window is created!");
        return false;
    }

    uint32 width      = m_window_configuration.get_width();
    uint32 height     = m_window_configuration.get_height();
    const char* title = m_window_configuration.get_title();

    // Hints valid for all windows
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window)
    {
        MANGO_LOG_ERROR("glfwCreateWindow failed! No window is created!");
        return false;
    }
    m_window_handle = static_cast<void*>(window);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    uint32 pos_x            = mode->width / 2 - width / 2;
    uint32 pos_y            = mode->height / 2 - height / 2;
    glfwSetWindowPos(window, pos_x, pos_y);

    MANGO_LOG_DEBUG("Window Position is ({0}, {1})", pos_x, pos_y);
    MANGO_LOG_DEBUG("Window Size is {0} x {1}", width, height);

    return true;
}

void linux_window_system::swap_buffers()
{
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    glfwSwapBuffers(static_cast<GLFWwindow*>(m_window_handle));
}

void linux_window_system::set_size(uint32 width, uint32 height)
{
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    m_window_configuration.set_width(width);
    m_window_configuration.set_height(height);
    glfwSetWindowSize(static_cast<GLFWwindow*>(m_window_handle), width, height);
}

void linux_window_system::configure(const window_configuration& configuration)
{
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    glfwDestroyWindow(static_cast<GLFWwindow*>(m_window_handle));

    m_window_configuration = configuration;

    uint32 width      = m_window_configuration.get_width();
    uint32 height     = m_window_configuration.get_height();
    const char* title = m_window_configuration.get_title();

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window)
    {
        MANGO_LOG_ERROR("glfwCreateWindow failed! No window is created!");
        return;
    }
    m_window_handle = static_cast<void*>(window);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    uint32 pos_x            = mode->width / 2 - width / 2;
    uint32 pos_y            = mode->height / 2 - height / 2;
    glfwSetWindowPos(window, pos_x, pos_y);

    MANGO_LOG_DEBUG("Window Position is ({0}, {1})", pos_x, pos_y);
    MANGO_LOG_DEBUG("Window Size is {0} x {1}", width, height);

#ifdef MANGO_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif // MANGO_DEBUG

    make_window_context_current();
    m_window_user_data.shared_context->set_gl_loading_procedure(reinterpret_cast<mango_gl_load_proc>(glfwGetProcAddress)); // TODO Paul: Should this be done here or before creating the gl context.
    glfwSetWindowUserPointer(window, static_cast<void*>(&m_window_user_data));

    {
        // Test just for fun and because it is nice to debug with resizing --- This is BAD. TODO Paul: Make this correct and fancy.
        glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int w, int h) {
            window_user_data* data = static_cast<window_user_data*>(glfwGetWindowUserPointer(window));
            context_impl* c        = data->shared_context.get();
            if (w > 0 && h > 0)
            {
                c->get_current_scene()->get_camera_component(1)->aspect = (float)w / (float)h; // We know that camera is entity 1... because this is dumb.
                c->get_render_system_internal().lock()->set_viewport(0, 0, w, h);
            }
        });
    }
}

void linux_window_system::update(float dt)
{
    MANGO_UNUSED(dt);
}

void linux_window_system::poll_events()
{
    glfwPollEvents();
}

bool linux_window_system::should_close()
{
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    return glfwWindowShouldClose(static_cast<GLFWwindow*>(m_window_handle));
}

void linux_window_system::set_vsync(bool enabled)
{
    make_window_context_current();
    glfwSwapInterval(enabled ? 1 : 0);
}

void linux_window_system::make_window_context_current()
{
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    glfwMakeContextCurrent(static_cast<GLFWwindow*>(m_window_handle));
}

void linux_window_system::set_drag_and_drop_callback(drag_n_drop_callback callback)
{
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    m_window_user_data.drag_n_drop_callback = callback;
    glfwSetDropCallback(static_cast<GLFWwindow*>(m_window_handle), [](GLFWwindow* window, int count, const char** paths) {
        window_user_data* data = static_cast<window_user_data*>(glfwGetWindowUserPointer(window));
        data->drag_n_drop_callback(count, paths);
    });
}

void linux_window_system::destroy()
{
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    glfwDestroyWindow(static_cast<GLFWwindow*>(m_window_handle));
    m_window_handle = nullptr;
    glfwTerminate();
}