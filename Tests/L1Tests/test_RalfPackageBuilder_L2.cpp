#include <gtest/gtest.h>
#include "RalfPackageBuilder.h"
#include "RalfSupport.h"
#include <fstream>

using namespace ralf;


// L2 TEST: Full integration flow

TEST(RalfPackageBuilderL2Test, GenerateDobbySpecBasicFlow)
{
    // Step 1: Create dummy JSON config file
    std::ofstream file("ralf_pkg.json");
    file << R"({
        "packages": [
            {
                "pkgMetaDataPath": "meta1",
                "pkgMountPath": "/mnt/pkg1"
            }
        ]
    })";
    file.close();

    // Step 2: Prepare runtimeConfigObject
    WPEFramework::Exchange::RuntimeConfig runtimeConfig;
    runtimeConfig.ralfPkgPath = "ralf_pkg.json";

    // Step 3: Prepare ApplicationConfiguration
    WPEFramework::Plugin::ApplicationConfiguration config;
    config.mAppInstanceId = "testApp";
    config.mUserId = 1000;
    config.mGroupId = 1000;

    RalfPackageBuilder builder;

    std::string dobbySpec;

    // Step 4: Call main function
    bool result = builder.generateRalfDobbySpec(config, runtimeConfig, dobbySpec);

    // Step 5: Validate behavior
    EXPECT_TRUE(result);
}