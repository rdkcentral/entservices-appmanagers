#include <gtest/gtest.h>

#include "VictimSelector.h"

namespace {

TEST(VictimSelectorL1, InitializeRejectsNullService)
{
    WPEFramework::Plugin::VictimSelector selector;

    EXPECT_EQ("VictimSelector received an invalid service", selector.Initialize(nullptr));
}

TEST(VictimSelectorL1, InformationReturnsPluginDescription)
{
    WPEFramework::Plugin::VictimSelector selector;

    EXPECT_EQ("Victim Selector plugin", selector.Information());
}

} // namespace