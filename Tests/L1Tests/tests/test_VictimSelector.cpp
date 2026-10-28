#include <gtest/gtest.h>

#include "VictimSelector.h"

namespace {

TEST(VictimSelectorL1, InitializeRejectsNullService)
{
    Thunder::Core::ProxyType<Thunder::Plugin::VictimSelector> selector =
        Thunder::Core::ProxyType<Thunder::Plugin::VictimSelector>::Create();

    EXPECT_EQ("VictimSelector received an invalid service", selector->Initialize(nullptr));
}

TEST(VictimSelectorL1, InformationReturnsPluginDescription)
{
    Thunder::Core::ProxyType<Thunder::Plugin::VictimSelector> selector =
        Thunder::Core::ProxyType<Thunder::Plugin::VictimSelector>::Create();

    EXPECT_EQ("Victim Selector plugin", selector->Information());
}

} // namespace