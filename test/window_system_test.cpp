//! \file      window_system_test.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <gtest/gtest.h>
#include <mango/mango.hpp>
#include "mock_classes.hpp"
#if defined(WIN32)
#include <core/win32_window_system.hpp>
#define platform_window_system_impl mango::win32_window_system
#elif defined(LINUX)
#include <core/linux_window_system.hpp>
#define platform_window_system_impl mango::linux_window_system
#endif

//! \cond NO_DOC

class window_system_test : public ::testing::Test
{
  protected:
    window_system_test() {}

    ~window_system_test() override {}

    void SetUp() override {}

    void TearDown() override {}
    mango::shared_ptr<platform_window_system_impl> m_window_system;
};

TEST_F(window_system_test, win32_window_system_no_failure_on_function_calls)
{
    ASSERT_NO_FATAL_FAILURE(m_window_system = mango::make_shared<platform_window_system_impl>());
    ASSERT_NE(nullptr, m_window_system);
    ASSERT_TRUE(m_window_system->create());
    mango::window_configuration window_config(100, 100, "Test");
    ASSERT_NO_FATAL_FAILURE(m_window_system->configure(window_config));
    ASSERT_NO_FATAL_FAILURE(m_window_system->update(0.0f));
    ASSERT_NO_FATAL_FAILURE(m_window_system->swap_buffers());
    ASSERT_NO_FATAL_FAILURE(m_window_system->poll_events());
    ASSERT_FALSE(m_window_system->should_close());
    ASSERT_NO_FATAL_FAILURE(m_window_system->destroy());
}

//! \endcond