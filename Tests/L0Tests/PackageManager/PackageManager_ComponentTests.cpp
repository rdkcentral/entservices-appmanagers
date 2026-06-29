#include <cstdio>
#include <iostream>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

#include "Module.h"
#include "HttpClient.h"
#include "PackageManagerTelemetryReporting.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"

namespace {
class CapturingTelemetryMetrics final : public WPEFramework::Exchange::ITelemetryMetrics {
public:
    CapturingTelemetryMetrics()
        : _refCount(1)
        , recordCalls(0)
        , publishCalls(0)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == remaining) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED : WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::ITelemetryMetrics::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::ITelemetryMetrics*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult Record(const std::string& id, const std::string& metrics, const std::string& name) override
    {
        lastId = id;
        lastMetrics = metrics;
        lastName = name;
        recordCalls++;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Publish(const std::string& id, const std::string& name) override
    {
        lastPublishId = id;
        lastPublishName = name;
        publishCalls++;
        return WPEFramework::Core::ERROR_NONE;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> recordCalls;
    std::atomic<uint32_t> publishCalls;
    std::string lastId;
    std::string lastName;
    std::string lastMetrics;
    std::string lastPublishId;
    std::string lastPublishName;
};
} // namespace

uint32_t Test_PM_Component_HttpClient_InvalidOutputPathReturnsDiskError()
{
    L0Test::TestResult tr;
    HttpClient client;

    const auto rc = client.downloadFile("http://127.0.0.1:1/unreachable", "/proc/forbidden_file", 0);
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(rc), static_cast<uint32_t>(HttpClient::Status::DiskError),
        "HttpClient::downloadFile() returns DiskError when destination file cannot be opened");

    return tr.failures;
}

uint32_t Test_PM_Component_HttpClient_InvalidUrlReturnsHttpError()
{
    L0Test::TestResult tr;
    HttpClient client;

    const std::string out = "/tmp/pm_l0_httpclient.bin";
    const auto rc = client.downloadFile("http://127.0.0.1:1/unreachable", out, 0);
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(rc), static_cast<uint32_t>(HttpClient::Status::HttpError),
        "HttpClient::downloadFile() returns HttpError for unreachable URL with writable output path");

    std::remove(out.c_str());
    return tr.failures;
}

uint32_t Test_PM_Component_HttpClient_InlineMethodsCoverage()
{
    L0Test::TestResult tr;
    HttpClient client;

    client.setRateLimit(128);
    client.cancel();
    client.pause();
    client.resume();

    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(client.getProgress()), 0, "HttpClient initial progress is zero");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(client.getStatusCode()), 0, "HttpClient initial status code is zero");

    trace_exit scope("HttpClientInline", 1);
    (void)scope;

    return tr.failures;
}

uint32_t Test_PM_Component_TelemetryReporting_Markers()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock::FakeTelemetryMetrics telemetry;
    L0Test::FakeStorageManager storage;
    L0Test::ServiceMock::FakeSubSystem subSystem;
    L0Test::ServiceMock service(L0Test::ServiceMock::Config(&storage, &subSystem, &telemetry));

    auto& reporting = WPEFramework::Plugin::PackageManagerTelemetryReporting::getInstance();
    reporting.initialize(&service);

    const uint64_t requestTime = static_cast<uint64_t>(reporting.getCurrentTimestampMs() - 5);

    reporting.recordAndPublishTelemetryData(TELEMETRY_MARKER_INSTALL_TIME, "app.id", requestTime, 0);
    reporting.recordAndPublishTelemetryData(TELEMETRY_MARKER_UNINSTALL_TIME, "app.id", requestTime, 0);
    reporting.recordAndPublishTelemetryData(TELEMETRY_MARKER_INSTALL_ERROR, "app.id", requestTime, 5);
    reporting.recordAndPublishTelemetryData(TELEMETRY_MARKER_UNINSTALL_ERROR, "app.id", requestTime, 6);
    reporting.recordAndPublishTelemetryData(TELEMETRY_MARKER_LAUNCH_TIME, "app.id", requestTime, 0, "runtime.app", "1.0.0");
    reporting.recordAndPublishTelemetryData(TELEMETRY_MARKER_CLOSE_TIME, "app.id", requestTime, 0);
    reporting.recordAndPublishTelemetryData("", "app.id", requestTime, 0);
    reporting.recordAndPublishTelemetryData("UNKNOWN_MARKER", "app.id", requestTime, 0);

    // For launch/close publish=false. For install/uninstall and error markers publish=true.
    L0Test::ExpectTrue(tr, telemetry.recordCalls.load() >= 6, "Telemetry record is invoked for known markers");
    L0Test::ExpectTrue(tr, telemetry.publishCalls.load() >= 4, "Telemetry publish is invoked for publish-enabled markers");

    reporting.reset();
    return tr.failures;
}

uint32_t Test_PM_Component_TelemetryReporting_PackageCacheMarkerPayload()
{
    L0Test::TestResult tr;

    CapturingTelemetryMetrics telemetry;
    L0Test::FakeStorageManager storage;
    L0Test::ServiceMock::FakeSubSystem subSystem;
    L0Test::ServiceMock service(L0Test::ServiceMock::Config(&storage, &subSystem, &telemetry));

    auto& reporting = WPEFramework::Plugin::PackageManagerTelemetryReporting::getInstance();
    reporting.initialize(&service);

    const uint64_t requestTime = static_cast<uint64_t>(reporting.getCurrentTimestampMs() - 10);
    reporting.recordAndPublishTelemetryData(
        TELEMETRY_MARKER_PACKAGE_CACHE_INIT_TIME,
        "CacheInitTimeInternal",
        requestTime,
        0,
        "",
        "",
        3,
        "sqlite");

    L0Test::ExpectEqU32(tr, telemetry.recordCalls.load(), 1U, "package cache marker should record exactly once");
    L0Test::ExpectEqU32(tr, telemetry.publishCalls.load(), 1U, "package cache marker should publish exactly once");
    L0Test::ExpectTrue(tr, telemetry.lastName == TELEMETRY_MARKER_PACKAGE_CACHE_INIT_TIME,
        "record marker name should match package cache marker");
    L0Test::ExpectTrue(tr, telemetry.lastPublishName == TELEMETRY_MARKER_PACKAGE_CACHE_INIT_TIME,
        "publish marker name should match package cache marker");
    L0Test::ExpectTrue(tr, telemetry.lastMetrics.find("\"packageManagerCacheInitTime\"") != std::string::npos,
        "payload should include packageManagerCacheInitTime key");
    L0Test::ExpectTrue(tr, telemetry.lastMetrics.find("\"count\":3") != std::string::npos,
        "payload should include numeric count key");
    L0Test::ExpectTrue(tr, telemetry.lastMetrics.find("\"backend\":\"sqlite\"") != std::string::npos,
        "payload should include backend key");

    reporting.reset();
    return tr.failures;
}
