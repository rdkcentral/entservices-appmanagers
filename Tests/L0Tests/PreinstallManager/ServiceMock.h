/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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

#pragma once

/**
 * @file ServiceMock.h
 * @brief PreinstallManager-specific fake objects and service mock for L0 tests.
 *
 * Provides:
 *   ServiceMock                      - IShell stub for PreinstallManager tests (extends L0ShellMock)
 *   FakePackageInstaller             - IPackageInstaller stub with configurable behaviour
 *   FakePackageIterator              - IPackageIterator stub backed by a std::vector
 *   FakePreinstallNotification       - IPreinstallManager::INotification call counter
 *   FakePreinstallManagerNoConfig    - IPreinstallManager stub (no IConfiguration — triggers L0-LC-005)
 *   FakePreinstallManagerWithConfig  - IPreinstallManager + IConfiguration stub (Configure => ERROR_NONE)
 *   FakePreinstallManagerBadConfig   - IPreinstallManager + IConfiguration stub (Configure => ERROR_GENERAL)
 */

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include <core/core.h>
#include <plugins/plugins.h>
#include <interfaces/IPreinstallManager.h>
#include <interfaces/IAppPackageManager.h>
#include <interfaces/IConfiguration.h>

#include "common/L0ShellMock.hpp"

namespace L0Test {

// ══════════════════════════════════════════════════════════════════════════════
// FakePackageIterator
// Implements Exchange::IPackageInstaller::IPackageIterator
// (= RPC::IIteratorType<Package, ID_PACKAGE_ITERATOR>)
// ══════════════════════════════════════════════════════════════════════════════

class FakePackageIterator final : public WPEFramework::Exchange::IPackageInstaller::IPackageIterator {
public:
    using Package = WPEFramework::Exchange::IPackageInstaller::Package;

    explicit FakePackageIterator(std::vector<Package> packages)
        : _packages(std::move(packages))
        , _idx(0)
        , _refCount(1)
    {
    }

    ~FakePackageIterator() override = default;

    // IUnknown
    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t /*id*/) override { return nullptr; }

    // IIteratorType
    bool Next(Package& pkg) override
    {
        if (_idx >= _packages.size()) {
            return false;
        }
        pkg = _packages[_idx++];
        return true;
    }

    bool Previous(Package& pkg) override
    {
        if (_idx == 0) return false;
        pkg = _packages[--_idx];
        return true;
    }

    void Reset(const uint32_t position) override
    {
        _idx = static_cast<size_t>(position);
    }

    bool IsValid() const override
    {
        return _idx < _packages.size();
    }

    uint32_t Count() const override
    {
        return static_cast<uint32_t>(_packages.size());
    }

    Package Current() const override
    {
        if (_idx > 0 && (_idx - 1) < _packages.size()) {
            return _packages[_idx - 1];
        }
        return Package{};
    }

private:
    std::vector<Package>         _packages;
    size_t                       _idx;
    mutable std::atomic<uint32_t> _refCount;
};


// ══════════════════════════════════════════════════════════════════════════════
// FakePackageInstaller
// Implements Exchange::IPackageInstaller with configurable behaviour.
// ══════════════════════════════════════════════════════════════════════════════

class FakePackageInstaller final : public WPEFramework::Exchange::IPackageInstaller {
public:
    using Package    = WPEFramework::Exchange::IPackageInstaller::Package;
    using InstallState  = WPEFramework::Exchange::IPackageInstaller::InstallState;
    using FailReason    = WPEFramework::Exchange::IPackageInstaller::FailReason;
    using INotification = WPEFramework::Exchange::IPackageInstaller::INotification;
    using IPackageIterator  = WPEFramework::Exchange::IPackageInstaller::IPackageIterator;
    using IKeyValueIterator = WPEFramework::Exchange::IPackageInstaller::IKeyValueIterator;
    using RuntimeConfig     = WPEFramework::Exchange::RuntimeConfig;

    FakePackageInstaller()
        : _refCount(1)
        , installShouldFail(false)
        , listShouldFail(false)
        , listPackagesReturnsNullIterator(false)
        , getConfigShouldFail(false)
        , failReasonToReturn(FailReason::NONE)
        , returnedPackageId("com.example.app")
        , returnedVersion("1.0.0")
        , installedPackages()
        , _storedNotification(nullptr)
    {
    }

    ~FakePackageInstaller() override = default;

    // IUnknown
    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPackageInstaller::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPackageInstaller*>(this);
        }
        return nullptr;
    }

    // IPackageInstaller – Register / Unregister
    WPEFramework::Core::hresult Register(INotification* sink) override
    {
        registerCalls++;
        _storedNotification = sink;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(INotification* /*sink*/) override
    {
        unregisterCalls++;
        _storedNotification = nullptr;
        return WPEFramework::Core::ERROR_NONE;
    }

    // IPackageInstaller – Install
    WPEFramework::Core::hresult Install(
        const std::string& /*packageId*/,
        const std::string& /*version*/,
        IKeyValueIterator* const& /*additionalMetadata*/,
        const std::string& /*fileLocator*/,
        FailReason& failReason) override
    {
        installCalls++;
        if (installShouldFail) {
            failReason = failReasonToReturn;
            return WPEFramework::Core::ERROR_GENERAL;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    // IPackageInstaller – Uninstall
    WPEFramework::Core::hresult Uninstall(
        const std::string& /*packageId*/,
        std::string& /*errorReason*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    // IPackageInstaller – ListPackages
    WPEFramework::Core::hresult ListPackages(IPackageIterator*& packages) override
    {
        listPackagesCalls++;
        if (listShouldFail) {
            return WPEFramework::Core::ERROR_GENERAL;
        }
        if (listPackagesReturnsNullIterator) {
            packages = nullptr;
            return WPEFramework::Core::ERROR_NONE;
        }
        // Build and return a FakePackageIterator from installedPackages
        auto* iter = new FakePackageIterator(installedPackages);
        packages = iter;
        return WPEFramework::Core::ERROR_NONE;
    }

    // IPackageInstaller – Config
    WPEFramework::Core::hresult Config(
        const std::string& /*packageId*/,
        const std::string& /*version*/,
        RuntimeConfig& /*configMetadata*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    // IPackageInstaller – PackageState
    WPEFramework::Core::hresult PackageState(
        const std::string& /*packageId*/,
        const std::string& /*version*/,
        InstallState& state) override
    {
        state = InstallState::INSTALLED;
        return WPEFramework::Core::ERROR_NONE;
    }

    // IPackageInstaller – GetConfigForPackage
    WPEFramework::Core::hresult GetConfigForPackage(
        const std::string& /*fileLocator*/,
        std::string& id,
        std::string& version,
        RuntimeConfig& /*config*/) override
    {
        getConfigCalls++;
        if (getConfigShouldFail) {
            return WPEFramework::Core::ERROR_GENERAL;
        }
        id      = returnedPackageId;
        version = returnedVersion;
        return WPEFramework::Core::ERROR_NONE;
    }

    // ── Test controls ────────────────────────────────────────────────────────
    bool        installShouldFail               { false };
    bool        listShouldFail                  { false };
    bool        listPackagesReturnsNullIterator  { false };
    bool        getConfigShouldFail             { false };
    FailReason  failReasonToReturn              { FailReason::NONE };
    std::string returnedPackageId               { "com.example.app" };
    std::string returnedVersion                 { "1.0.0" };

    // Pre-populate with packages that ListPackages() returns
    std::vector<Package> installedPackages;

    // ── Observability ────────────────────────────────────────────────────────
    mutable std::atomic<uint32_t> _refCount        { 1 };
    std::atomic<uint32_t>  registerCalls       { 0 };
    std::atomic<uint32_t>  unregisterCalls     { 0 };
    std::atomic<uint32_t>  installCalls        { 0 };
    std::atomic<uint32_t>  listPackagesCalls   { 0 };
    std::atomic<uint32_t>  getConfigCalls      { 0 };

    INotification* GetStoredNotification() const { return _storedNotification; }

private:
    INotification* _storedNotification;
};


// ══════════════════════════════════════════════════════════════════════════════
// FakePreinstallNotification
// Implements Exchange::IPreinstallManager::INotification
// ══════════════════════════════════════════════════════════════════════════════

class FakePreinstallNotification final
    : public WPEFramework::Exchange::IPreinstallManager::INotification {
public:
    FakePreinstallNotification()
        : _refCount(1)
        , onAppInstallationStatusCount(0)
    {
    }

    ~FakePreinstallNotification() override = default;

    // IUnknown
    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == n) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED
                         : WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPreinstallManager::INotification::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPreinstallManager::INotification*>(this);
        }
        return nullptr;
    }

    // IPreinstallManager::INotification
    void OnAppInstallationStatus(const std::string& jsonresponse) override
    {
        onAppInstallationStatusCount++;
        lastJsonResponse = jsonresponse;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t>         onAppInstallationStatusCount { 0 };
    std::string                   lastJsonResponse;
};


// ══════════════════════════════════════════════════════════════════════════════
// FakePreinstallManagerNoConfig
// Implements IPreinstallManager only — no IConfiguration.
// Injected via InstantiateHandler to trigger L0-LC-005.
// ══════════════════════════════════════════════════════════════════════════════

class FakePreinstallManagerNoConfig final
    : public WPEFramework::Exchange::IPreinstallManager {
public:
    FakePreinstallManagerNoConfig() : _refCount(1) {}
    ~FakePreinstallManagerNoConfig() override = default;

    // IUnknown
    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPreinstallManager::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPreinstallManager*>(this);
        }
        // Deliberately does NOT expose IConfiguration
        return nullptr;
    }

    WPEFramework::Core::hresult Register(
        WPEFramework::Exchange::IPreinstallManager::INotification*) override
        { return WPEFramework::Core::ERROR_NONE; }

    WPEFramework::Core::hresult Unregister(
        WPEFramework::Exchange::IPreinstallManager::INotification*) override
        { return WPEFramework::Core::ERROR_NONE; }

    WPEFramework::Core::hresult StartPreinstall(bool) override
        { return WPEFramework::Core::ERROR_NONE; }

private:
    mutable std::atomic<uint32_t> _refCount;
};


// ══════════════════════════════════════════════════════════════════════════════
// FakePreinstallManagerWithConfig
// Implements IPreinstallManager + IConfiguration (Configure => ERROR_NONE).
// Used in L0-LC-003 (Initialize success).
// ══════════════════════════════════════════════════════════════════════════════

class FakePreinstallManagerWithConfig final
    : public WPEFramework::Exchange::IPreinstallManager
    , public WPEFramework::Exchange::IConfiguration {
public:
    FakePreinstallManagerWithConfig() : _refCount(1) {}
    ~FakePreinstallManagerWithConfig() override = default;

    // IUnknown
    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPreinstallManager::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPreinstallManager*>(this);
        }
        if (id == WPEFramework::Exchange::IConfiguration::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IConfiguration*>(this);
        }
        return nullptr;
    }

    // IPreinstallManager
    WPEFramework::Core::hresult Register(
        WPEFramework::Exchange::IPreinstallManager::INotification* n) override
    {
        _notification = n;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(
        WPEFramework::Exchange::IPreinstallManager::INotification*) override
    {
        _notification = nullptr;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult StartPreinstall(bool) override
        { return WPEFramework::Core::ERROR_NONE; }

    // IConfiguration
    uint32_t Configure(WPEFramework::PluginHost::IShell*) override
        { return WPEFramework::Core::ERROR_NONE; }

private:
    mutable std::atomic<uint32_t>                   _refCount;
    WPEFramework::Exchange::IPreinstallManager::INotification* _notification { nullptr };
};


// ══════════════════════════════════════════════════════════════════════════════
// FakePreinstallManagerBadConfig
// Implements IPreinstallManager + IConfiguration (Configure => ERROR_GENERAL).
// Used in L0-LC-006 (Initialize fails when Configure() returns error).
// ══════════════════════════════════════════════════════════════════════════════

class FakePreinstallManagerBadConfig final
    : public WPEFramework::Exchange::IPreinstallManager
    , public WPEFramework::Exchange::IConfiguration {
public:
    FakePreinstallManagerBadConfig() : _refCount(1) {}
    ~FakePreinstallManagerBadConfig() override = default;

    void AddRef() const override
        { _refCount.fetch_add(1, std::memory_order_relaxed); }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPreinstallManager::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPreinstallManager*>(this);
        }
        if (id == WPEFramework::Exchange::IConfiguration::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IConfiguration*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult Register(
        WPEFramework::Exchange::IPreinstallManager::INotification*) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unregister(
        WPEFramework::Exchange::IPreinstallManager::INotification*) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StartPreinstall(bool) override
        { return WPEFramework::Core::ERROR_NONE; }

    // IConfiguration – deliberately returns error
    uint32_t Configure(WPEFramework::PluginHost::IShell*) override
        { return WPEFramework::Core::ERROR_GENERAL; }

private:
    mutable std::atomic<uint32_t> _refCount;
};


// ══════════════════════════════════════════════════════════════════════════════
// ServiceMock
// IShell stub for PreinstallManager L0 tests.
// Extends L0ShellMock and overrides QueryInterfaceByCallsign to inject the
// FakePackageInstaller for "org.rdk.PackageManagerRDKEMS".
// ══════════════════════════════════════════════════════════════════════════════

class ServiceMock final : public L0Test::L0ShellMock {
public:
    explicit ServiceMock(FakePackageInstaller* pm = nullptr)
        : L0Test::L0ShellMock()
        , _pm(pm)
    {
        SetCallsign("org.rdk.PreinstallManager");
    }

    ~ServiceMock() override = default;

    /** Set (or replace) the FakePackageInstaller returned for the PM callsign. */
    void SetPackageInstaller(FakePackageInstaller* pm) { _pm = pm; }

    void* QueryInterfaceByCallsign(const uint32_t id,
                                   const std::string& callsign) override
    {
        if (callsign == "org.rdk.PackageManagerRDKEMS"
            && id == WPEFramework::Exchange::IPackageInstaller::ID
            && _pm != nullptr)
        {
            _pm->AddRef();
            return static_cast<void*>(_pm);
        }
        return nullptr;
    }

private:
    FakePackageInstaller* _pm;
};


// ══════════════════════════════════════════════════════════════════════════════
// Helper: PluginAndService RAII wrapper for lifecycle tests
// ══════════════════════════════════════════════════════════════════════════════

struct PluginAndService {
    L0Test::ServiceMock           service;
    WPEFramework::PluginHost::IPlugin* plugin { nullptr };

    PluginAndService()
        : service()
        , plugin(WPEFramework::Core::Service<WPEFramework::Plugin::PreinstallManager>::Create<
                 WPEFramework::PluginHost::IPlugin>())
    {
    }

    ~PluginAndService()
    {
        if (nullptr != plugin) {
            plugin->Release();
            plugin = nullptr;
        }
    }

    PluginAndService(const PluginAndService&)            = delete;
    PluginAndService& operator=(const PluginAndService&) = delete;
};

} // namespace L0Test
