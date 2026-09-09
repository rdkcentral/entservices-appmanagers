#include <gtest/gtest.h>

#include "VictimSelector.h"

namespace {

TEST(VictimSelectorL1, InitializeRejectsNullService)
{
    WPEFramework::Core::ProxyType<WPEFramework::Plugin::VictimSelector> selector =
        WPEFramework::Core::ProxyType<WPEFramework::Plugin::VictimSelector>::Create();

    EXPECT_EQ("VictimSelector received an invalid service", selector->Initialize(nullptr));
}

TEST(VictimSelectorL1, InformationReturnsPluginDescription)
{
    WPEFramework::Core::ProxyType<WPEFramework::Plugin::VictimSelector> selector =
        WPEFramework::Core::ProxyType<WPEFramework::Plugin::VictimSelector>::Create();

    EXPECT_EQ("Victim Selector plugin", selector->Information());
}

} // namespace