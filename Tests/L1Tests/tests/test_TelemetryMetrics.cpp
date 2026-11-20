/*
 * Copyright 2026 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include "Rfc.h"
#include "Telemetry.h"
#include "TelemetryMetricsImplementation.h"

using namespace WPEFramework;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrEq;

namespace {
class MetricsRfcMock : public RfcApiImpl {
public:
    MOCK_METHOD(WDMP_STATUS, getRFCParameter, (char*, const char*, RFC_ParamData_t*), (override));
    MOCK_METHOD(WDMP_STATUS, setRFCParameter, (char*, const char*, const char*, DATA_TYPE), (override));
    MOCK_METHOD(const char*, getRFCErrorString, (WDMP_STATUS), (override));
};

class MetricsTelemetryMock : public TelemetryApiImpl {
public:
    MOCK_METHOD(void, t2_init, (char*), (override));
    MOCK_METHOD(T2ERROR, t2_event_s, (char*, char*), (override));
};

class TelemetryMetricsTest : public ::testing::Test {
protected:
    NiceMock<MetricsRfcMock> rfc;
    NiceMock<MetricsTelemetryMock> telemetry;
    Exchange::ITelemetryMetrics* implementation = nullptr;

    void SetUp() override
    {
        RfcApi::setImpl(&rfc);
        TelemetryApi::setImpl(&telemetry);
        implementation = Core::Service<Plugin::TelemetryMetricsImplementation>::Create<Exchange::ITelemetryMetrics>();
        ASSERT_NE(nullptr, implementation);
    }

    void TearDown() override
    {
        if (nullptr != implementation) {
            implementation->Release();
        }
        TelemetryApi::setImpl(nullptr);
        RfcApi::setImpl(nullptr);
    }
};

TEST_F(TelemetryMetricsTest, NonVerbosePublishesWithoutReadingRfc)
{
    EXPECT_CALL(rfc, getRFCParameter(_, _, _)).Times(0);
    EXPECT_CALL(telemetry, t2_event_s(StrEq("customMarker"), StrEq("{\"event\":\"launch\"}")))
        .WillOnce(Return(T2ERROR_SUCCESS));
    EXPECT_EQ(Core::ERROR_NONE, implementation->PublishEvent("{\"event\":\"launch\"}", "customMarker", false));
}

struct VerboseRfcCase {
    WDMP_STATUS status;
    const char* value;
    bool publish;
};
class TelemetryMetricsVerboseTest : public TelemetryMetricsTest,
                                    public ::testing::WithParamInterface<VerboseRfcCase> {};

TEST_P(TelemetryMetricsVerboseTest, PublishesOnlyWhenRfcIsEnabled)
{
    const auto testCase = GetParam();
    EXPECT_CALL(rfc, getRFCParameter(nullptr, StrEq("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.RDKAppManager.ObservabilityVerboseEnabled"), _))
        .WillOnce([testCase](char*, const char*, RFC_ParamData_t* param) {
            std::strcpy(param->value, testCase.value);
            param->type = WDMP_BOOLEAN;
            return testCase.status;
        });
    EXPECT_CALL(telemetry, t2_event_s(StrEq("marker"), StrEq("payload")))
        .Times(testCase.publish ? 1 : 0).WillRepeatedly(Return(T2ERROR_SUCCESS));
    EXPECT_EQ(Core::ERROR_NONE, implementation->PublishEvent("payload", "marker", true));
}

INSTANTIATE_TEST_SUITE_P(RfcValues, TelemetryMetricsVerboseTest, ::testing::Values(
    VerboseRfcCase{WDMP_SUCCESS, "true", true},
    VerboseRfcCase{WDMP_SUCCESS, "1", true},
    VerboseRfcCase{WDMP_SUCCESS, "false", false},
    VerboseRfcCase{WDMP_SUCCESS, "0", false},
    VerboseRfcCase{WDMP_ERR_DEFAULT_VALUE, "true", true},
    VerboseRfcCase{WDMP_ERR_NOT_EXIST, "", false},
    VerboseRfcCase{WDMP_FAILURE, "true", false},
    VerboseRfcCase{WDMP_SUCCESS, "", false},
    VerboseRfcCase{WDMP_SUCCESS, "invalid", false}));

TEST_F(TelemetryMetricsTest, HonorsRfcChangesBetweenCalls)
{
    EXPECT_CALL(rfc, getRFCParameter(nullptr, StrEq("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.RDKAppManager.ObservabilityVerboseEnabled"), _))
        .WillOnce([](char*, const char*, RFC_ParamData_t* param) {
            std::strcpy(param->value, "false");
            return WDMP_SUCCESS;
        })
        .WillOnce([](char*, const char*, RFC_ParamData_t* param) {
            std::strcpy(param->value, "true");
            return WDMP_SUCCESS;
        });
    EXPECT_CALL(telemetry, t2_event_s(StrEq("marker"), StrEq("payload")))
        .WillOnce(Return(T2ERROR_SUCCESS));
    EXPECT_EQ(Core::ERROR_NONE, implementation->PublishEvent("payload", "marker", true));
    EXPECT_EQ(Core::ERROR_NONE, implementation->PublishEvent("payload", "marker", true));
}

TEST_F(TelemetryMetricsTest, ReportsT2Failure)
{
    EXPECT_CALL(rfc, getRFCParameter(_, _, _)).Times(0);
    EXPECT_CALL(telemetry, t2_event_s(StrEq("marker"), StrEq("payload")))
        .WillOnce(Return(T2ERROR_FAILURE));
    EXPECT_EQ(Core::ERROR_GENERAL, implementation->PublishEvent("payload", "marker", false));
}
} // namespace
