#!/usr/bin/env python3
import argparse
import json
import os
import re
from pathlib import Path
from typing import Dict, List

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')

DEFAULT_SCOPE_FOLDERS = [
    "AppManager",
    "AppStorageManager",
    "DownloadManager",
    "LifecycleManager",
    "PackageManager",
    "PreinstallManager",
    "RDKWindowManager",
    "TelemetryMetrics",
]

SYSTEM_HEADER_ALLOWLIST = {
    "algorithm",
    "array",
    "atomic",
    "chrono",
    "condition_variable",
    "cstdint",
    "cstddef",
    "cstdbool",
    "cstdarg",
    "cinttypes",
    "cstdio",
    "cstdlib",
    "cstring",
    "exception",
    "fstream",
    "functional",
    "future",
    "iomanip",
    "iostream",
    "iterator",
    "limits",
    "list",
    "map",
    "memory",
    "mutex",
    "new",
    "optional",
    "queue",
    "set",
    "sstream",
    "stack",
    "stdint.h",
    "stddef.h",
    "stdbool.h",
    "stdio.h",
    "stdlib.h",
    "string",
    "string.h",
    "thread",
    "time.h",
    "tuple",
    "type_traits",
    "unordered_map",
    "unordered_set",
    "utility",
    "variant",
    "vector",
}

FORCE_FALLBACK_HEADERS = {
    # Real helpers/UtilsString.h pulls many Thunder-specific types; keep lightweight stub in fuzz builds.
    "UtilsString.h",
    # Provide a deterministic cross-plugin module wrapper instead of folder-specific Module.h variants.
    "Module.h",
}


def should_skip_stub_generation(include_path: str) -> bool:
    # Skip standard/system headers to avoid shadowing libc/libc++.
    if include_path in SYSTEM_HEADER_ALLOWLIST:
        return True
    if include_path.startswith("sys/"):
        return True
    if "/" not in include_path and include_path.islower():
        return True
    return False


def parse_scope(scope_arg: str) -> List[str]:
    scope = [s.strip() for s in scope_arg.split(",") if s.strip()]
    return sorted(set(scope))


def collect_includes(repo_root: Path, include_folders: List[str]) -> List[str]:
    includes = set()
    for folder in include_folders:
        scope_root = repo_root / folder
        if not scope_root.exists() or not scope_root.is_dir():
            continue
        for path in scope_root.rglob("*"):
            if path.suffix not in {".cpp", ".h", ".cc", ".hpp"}:
                continue
            text = path.read_text(encoding="utf-8", errors="ignore")
            for line in text.splitlines():
                m = INCLUDE_RE.match(line)
                if m:
                    includes.add(m.group(1))
    # Transitive include used from dependency Module.h
    includes.add("interfaces/entservices_errorcodes.h")
    return sorted(includes)


def resolve_missing_headers(repo_root: Path, dep_root: Path, includes: List[str], include_folders: List[str]) -> Dict[str, str]:
    """
    Resolve headers: check repo root first, then dependencies folder.
    Only mark as generated-fallback if truly missing from both.
    Returns mapping of include path to actual file location (or <generated-fallback> if missing).
    """
    missing = {}
    for inc in includes:
        if inc in FORCE_FALLBACK_HEADERS:
            missing[inc] = "<generated-fallback>"
            continue
        # Check repo root first
        repo_candidate = repo_root / inc
        if repo_candidate.exists():
            missing[inc] = str(repo_candidate)
            continue

        # Resolve unqualified local includes from scoped folders and common shared dirs.
        if "/" not in inc:
            local_candidates = []
            for folder in include_folders:
                local_candidates.append(repo_root / folder / inc)
            local_candidates.append(repo_root / "helpers" / inc)
            local_candidates.append(repo_root / "interfaces" / inc)
            for c in local_candidates:
                if c.exists():
                    missing[inc] = str(c)
                    break
            if inc in missing:
                continue
        
        found = False
        if dep_root.exists():
            # Try exact path match first (e.g., "interfaces/IAppManager.h")
            dep_exact = dep_root / inc
            if dep_exact.exists():
                missing[inc] = str(dep_exact)
                found = True
            else:
                # Handle "interfaces/IXxx.h" pattern by looking in apis directories
                if inc.startswith("interfaces/"):
                    filename = Path(inc).name  # e.g., "ILifecycleManager.h"
                    # Search in entservices-apis/apis/* directories
                    apis_root = dep_root / "entservices-apis" / "apis"
                    if apis_root.exists():
                        for api_dir in apis_root.iterdir():
                            if api_dir.is_dir():
                                candidate = api_dir / filename
                                if candidate.exists():
                                    missing[inc] = str(candidate)
                                    found = True
                                    break
                
                # Fallback: search by filename anywhere in dependencies
                if not found:
                    filename = Path(inc).name
                    for dep_dir in dep_root.rglob("*"):
                        if dep_dir.is_file() and dep_dir.name == filename:
                            missing[inc] = str(dep_dir)
                            found = True
                            break
        
        # Only create stub if truly missing
        if not found:
            missing[inc] = "<generated-fallback>"
    
    return missing


def purge_collected_include_entries(stubs_dir: Path, includes: List[str]) -> None:
    """Remove stale generated include entries for collected includes before regenerating."""
    include_root = stubs_dir / "include"
    include_root.mkdir(parents=True, exist_ok=True)
    for inc in includes:
        header_path = include_root / inc
        if header_path.is_symlink() or header_path.exists():
            try:
                header_path.unlink()
            except Exception:
                pass


def write_profile_stubs(stubs_dir: Path) -> None:
    header = stubs_dir / "fuzz_stub_profiles.h"
    source = stubs_dir / "fuzz_stub_profiles.c"

    header.write_text(
        """#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FUZZ_VALID_RETURN = 0,
    FUZZ_NULL_RETURN = 1,
    FUZZ_ALLOCATION_FAILURE = 2,
    FUZZ_TIMEOUT = 3,
    FUZZ_INVALID_STATE = 4,
    FUZZ_MALFORMED_RESPONSE = 5,
    FUZZ_RPC_FAILURE = 6,
} FailureProfile;

FailureProfile CurrentProfile(void);
int ShouldFailAllocation(void);
int ShouldTimeout(void);
int ShouldReturnNull(void);
const char* FakeAppIdentifier(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif
""",
        encoding="utf-8",
    )

    source.write_text(
        """#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz_stub_profiles.h"

// Global stubs for undefined symbols - weak to avoid ODR violations when linked multiple times
__attribute__((weak)) int gCurrentFramerate = 60;

FailureProfile CurrentProfile(void) {
    const char* env = getenv("STUB_PROFILE");
    if (env == NULL) {
        return FUZZ_VALID_RETURN;
    }
    int v = atoi(env);
    if (v < 0 || v > 6) {
        return FUZZ_VALID_RETURN;
    }
    return (FailureProfile)v;
}

int ShouldFailAllocation(void) {
    return CurrentProfile() == FUZZ_ALLOCATION_FAILURE;
}

int ShouldTimeout(void) {
    return CurrentProfile() == FUZZ_TIMEOUT;
}

int ShouldReturnNull(void) {
    return CurrentProfile() == FUZZ_NULL_RETURN;
}

const char* FakeAppIdentifier(void) {
    return "org.rdk.fuzz.auto.generated";
}
""",
        encoding="utf-8",
    )


def write_missing_header_stubs(stubs_dir: Path, missing_headers: Dict[str, str]) -> None:
    """Write stub headers ONLY for truly missing headers, not for those resolved from dependencies."""
    include_root = stubs_dir / "include"
    include_root.mkdir(parents=True, exist_ok=True)

    for inc in sorted(missing_headers):
        resolved_from = missing_headers[inc]
        if should_skip_stub_generation(inc):
            continue
        # Only create stub if it wasn't resolved from dependencies
        if resolved_from == "<generated-fallback>":
            header_path = include_root / inc
            header_path.parent.mkdir(parents=True, exist_ok=True)
            
            # Skip if already exists (might be a symlink)
            if header_path.exists() or header_path.is_symlink():
                continue
            
            guard = re.sub(r"[^A-Za-z0-9]", "_", inc).upper() + "_FUZZ_STUB_H"
            if "json/json.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <map>\n"
                    "#include <memory>\n"
                    "#include <string>\n"
                    "#include <vector>\n"
                    "\n"
                    "namespace Json {\n"
                    "enum ValueType { nullValue = 0, intValue, uintValue, realValue, stringValue, booleanValue, arrayValue, objectValue };\n"
                    "\n"
                    "class Value {\n"
                    "public:\n"
                    "    Value() : _type(objectValue) {}\n"
                    "    Value(ValueType type) : _type(type) {}\n"
                    "    Value(const char* value) : _type(stringValue), _string(value ? value : \"\") {}\n"
                    "    Value(const std::string& value) : _type(stringValue), _string(value) {}\n"
                    "    Value(bool value) : _type(booleanValue), _bool(value) {}\n"
                    "    Value& operator=(const Value&) = default;\n"
                    "    Value& operator=(const std::string& value) { _type = stringValue; _string = value; return *this; }\n"
                    "    Value& operator=(const char* value) { _type = stringValue; _string = value ? value : \"\"; return *this; }\n"
                    "    Value& operator=(bool value) { _type = booleanValue; _bool = value; return *this; }\n"
                    "    Value& operator[](const std::string& key) { _type = objectValue; return _members[key]; }\n"
                    "    const Value& operator[](const std::string& key) const { static Value empty; auto it = _members.find(key); return (it != _members.end()) ? it->second : empty; }\n"
                    "    bool isObject() const { return true; }\n"
                    "    bool isMember(const std::string& key) const { return _members.find(key) != _members.end(); }\n"
                    "    std::vector<std::string> getMemberNames() const { std::vector<std::string> names; for (const auto& entry : _members) { names.push_back(entry.first); } return names; }\n"
                    "    std::string asString() const { return _string; }\n"
                    "    std::string toStyledString() const { return \"{}\"; }\n"
                    "private:\n"
                    "    ValueType _type;\n"
                    "    std::string _string;\n"
                    "    bool _bool{false};\n"
                    "    std::map<std::string, Value> _members;\n"
                    "};\n"
                    "\n"
                    "class CharReader {\n"
                    "public:\n"
                    "    virtual ~CharReader() = default;\n"
                    "    virtual bool parse(const char*, const char*, Value* root, std::string* errs) { if (nullptr != root) { *root = Value(objectValue); } if (nullptr != errs) { errs->clear(); } return true; }\n"
                    "};\n"
                    "\n"
                    "class CharReaderBuilder {\n"
                    "public:\n"
                    "    std::unique_ptr<CharReader> newCharReader() const { return std::unique_ptr<CharReader>(new CharReader()); }\n"
                    "};\n"
                    "\n"
                    "class Reader {\n"
                    "public:\n"
                    "    bool parse(const std::string&, Value&) { return true; }\n"
                    "};\n"
                    "\n"
                    "class StreamWriterBuilder {\n"
                    "public:\n"
                    "    Value& operator[](const std::string& key) { return _settings[key]; }\n"
                    "private:\n"
                    "    std::map<std::string, Value> _settings;\n"
                    "};\n"
                    "\n"
                    "inline std::string writeString(const StreamWriterBuilder&, const Value&) { return \"{}\"; }\n"
                    "} // namespace Json\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "interfaces/IAppManager.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <core/core.h>\n"
                    "#include <string>\n"
                    "#include <vector>\n"
                    "#include <stdint.h>\n"
                    "\n"
                    "namespace WPEFramework {\n"
                    "namespace Exchange {\n"
                    "\n"
                    "class IAppManager : public Core::IUnknown {\n"
                    "public:\n"
                    "    enum AppLifecycleState {\n"
                    "        APP_STATE_UNKNOWN = 0,\n"
                    "        APP_STATE_UNLOADED = 1,\n"
                    "        APP_STATE_LOADING = 2,\n"
                    "        APP_STATE_INITIALIZING = 3,\n"
                    "        APP_STATE_ACTIVE = 4,\n"
                    "        APP_STATE_PAUSED = 5,\n"
                    "        APP_STATE_SUSPENDED = 6,\n"
                    "        APP_STATE_HIBERNATED = 7,\n"
                    "        APP_STATE_TERMINATING = 8\n"
                    "    };\n"
                    "\n"
                    "    enum AppErrorReason {\n"
                    "        APP_ERROR_NONE = 0,\n"
                    "        APP_ERROR_PACKAGE_LOCK = 1,\n"
                    "        APP_ERROR_NOT_INSTALLED = 2,\n"
                    "        APP_ERROR_INTERNAL = 3\n"
                    "    };\n"
                    "\n"
                    "    struct LoadedAppInfo {\n"
                    "        std::string appId;\n"
                    "        std::string appInstanceId;\n"
                    "        AppLifecycleState state;\n"
                    "\n"
                    "        LoadedAppInfo()\n"
                    "            : appId(), appInstanceId(), state(APP_STATE_UNKNOWN) {}\n"
                    "    };\n"
                    "\n"
                    "    class ILoadedAppInfoIterator {\n"
                    "    public:\n"
                    "        virtual ~ILoadedAppInfoIterator() = default;\n"
                    "        virtual uint32_t AddRef() { return 1; }\n"
                    "        virtual uint32_t Release() { return 1; }\n"
                    "        virtual bool Next(LoadedAppInfo& out) {\n"
                    "            (void)out;\n"
                    "            return false;\n"
                    "        }\n"
                    "    };\n"
                    "\n"
                    "    class INotification {\n"
                    "    public:\n"
                    "        virtual ~INotification() = default;\n"
                    "        virtual void OnAppInstalled(const std::string&, const std::string&) {}\n"
                    "        virtual void OnAppUninstalled(const std::string&) {}\n"
                    "        virtual void OnAppLifecycleStateChanged(const std::string&, const std::string&, AppLifecycleState, AppLifecycleState, AppErrorReason) {}\n"
                    "        virtual void OnAppLaunchRequest(const std::string&, const std::string&, const std::string&) {}\n"
                    "        virtual void OnAppUnloaded(const std::string&, const std::string&) {}\n"
                    "    };\n"
                    "\n"
                    "    virtual Core::hresult Register(INotification*) { return Core::ERROR_NONE; }\n"
                    "    virtual Core::hresult Unregister(INotification*) { return Core::ERROR_NONE; }\n"
                    "};\n"
                    "\n"
                    "} // namespace Exchange\n"
                    "} // namespace WPEFramework\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "interfaces/ILifecycleManager.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <core/core.h>\n"
                    "#include <string>\n"
                    "\n"
                    "namespace WPEFramework {\n"
                    "namespace Exchange {\n"
                    "\n"
                    "struct RuntimeConfig {\n"
                    "    std::string runtimeType;\n"
                    "    RuntimeConfig() : runtimeType() {}\n"
                    "};\n"
                    "\n"
                    "class ILifecycleManager : public Core::IUnknown {\n"
                    "public:\n"
                    "    enum LifecycleState {\n"
                    "        UNLOADED = 0,\n"
                    "        LOADING = 1,\n"
                    "        INITIALIZING = 2,\n"
                    "        ACTIVE = 3,\n"
                    "        PAUSED = 4,\n"
                    "        SUSPENDED = 5,\n"
                    "        HIBERNATED = 6,\n"
                    "        TERMINATING = 7\n"
                    "    };\n"
                    "\n"
                    "    class INotification {\n"
                    "    public:\n"
                    "        virtual ~INotification() = default;\n"
                    "        virtual void OnAppLifecycleStateChanged(const std::string&, const std::string&, LifecycleState, LifecycleState, const std::string&) {}\n"
                    "        virtual void OnAppStateChanged(const std::string&, LifecycleState, const std::string&) {}\n"
                    "    };\n"
                    "\n"
                    "    virtual Core::hresult Register(INotification*) { return Core::ERROR_NONE; }\n"
                    "    virtual Core::hresult Unregister(INotification*) { return Core::ERROR_NONE; }\n"
                    "    virtual Core::hresult GetLoadedApps(bool, std::string&) { return Core::ERROR_NONE; }\n"
                    "    virtual Core::hresult IsAppLoaded(const std::string&, bool&) const { return Core::ERROR_NONE; }\n"
                    "    virtual Core::hresult SpawnApp(const std::string&, const std::string&, LifecycleState, const RuntimeConfig&, const std::string&, std::string&, std::string&, bool&) { return Core::ERROR_NONE; }\n"
                    "    virtual Core::hresult SetTargetAppState(const std::string&, LifecycleState, const std::string&) { return Core::ERROR_NONE; }\n"
                    "    virtual Core::hresult UnloadApp(const std::string&, std::string&, bool&) { return Core::ERROR_NONE; }\n"
                    "    virtual Core::hresult KillApp(const std::string&, std::string&, bool&) { return Core::ERROR_NONE; }\n"
                    "    virtual Core::hresult SendIntentToActiveApp(const std::string&, const std::string&, std::string&, bool&) { return Core::ERROR_NONE; }\n"
                    "};\n"
                    "\n"
                    "} // namespace Exchange\n"
                    "} // namespace WPEFramework\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "messaging/messaging.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "namespace WPEFramework {\n"
                    "namespace Core {\n"
                    "class MessagingClient {\n"
                    "public:\n"
                    "    MessagingClient() = default;\n"
                    "};\n"
                    "} // namespace Core\n"
                    "} // namespace WPEFramework\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "Module.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#ifndef MODULE_NAME\n"
                    "#define MODULE_NAME Interfaces\n"
                    "#endif\n"
                    "\n"
                    "#include <core/core.h>\n"
                    "#include <plugins/IPlugin.h>\n"
                    "#include <plugins/ISubSystem.h>\n"
                    "#include <plugins/IShell.h>\n"
                    "#include <plugins/IStateControl.h>\n"
                    "#include <interfaces/Ids.h>\n"
                    "#include <interfaces/entservices_errorcodes.h>\n"
                    "\n"
                    "#ifndef EXTERNAL\n"
                    "#define EXTERNAL\n"
                    "#endif\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "curl/curl.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <stdarg.h>\n"
                    "#include <stddef.h>\n"
                    "\n"
                    "#ifdef __cplusplus\n"
                    "extern \"C\" {\n"
                    "#endif\n"
                    "\n"
                    "typedef struct CURL CURL;\n"
                    "typedef int CURLcode;\n"
                    "typedef int CURLoption;\n"
                    "typedef int CURLINFO;\n"
                    "typedef long long curl_off_t;\n"
                    "\n"
                    "enum {\n"
                    "    CURLE_OK = 0,\n"
                    "    CURLE_WRITE_ERROR = 23\n"
                    "};\n"
                    "\n"
                    "enum {\n"
                    "    CURLPAUSE_RECV = 1 << 0,\n"
                    "    CURLPAUSE_SEND = 1 << 1,\n"
                    "    CURLPAUSE_CONT = 0\n"
                    "};\n"
                    "\n"
                    "enum {\n"
                    "    CURLOPT_URL = 10000,\n"
                    "    CURLOPT_MAX_RECV_SPEED_LARGE = 10001,\n"
                    "    CURLOPT_WRITEFUNCTION = 10002,\n"
                    "    CURLOPT_WRITEDATA = 10003,\n"
                    "    CURLOPT_NOPROGRESS = 10004,\n"
                    "    CURLOPT_PROGRESSDATA = 10005,\n"
                    "    CURLOPT_PROGRESSFUNCTION = 10006\n"
                    "};\n"
                    "\n"
                    "enum {\n"
                    "    CURLINFO_RESPONSE_CODE = 20000\n"
                    "};\n"
                    "\n"
                    "struct CURL {\n"
                    "    int _fuzz_dummy;\n"
                    "};\n"
                    "\n"
                    "static inline CURL* curl_easy_init(void) {\n"
                    "    static CURL instance;\n"
                    "    return &instance;\n"
                    "}\n"
                    "\n"
                    "static inline void curl_easy_cleanup(CURL* curl) {\n"
                    "    (void)curl;\n"
                    "}\n"
                    "\n"
                    "static inline CURLcode curl_easy_pause(CURL* curl, int bitmask) {\n"
                    "    (void)curl;\n"
                    "    (void)bitmask;\n"
                    "    return CURLE_OK;\n"
                    "}\n"
                    "\n"
                    "static inline CURLcode curl_easy_setopt(CURL* curl, CURLoption option, ...) {\n"
                    "    (void)curl;\n"
                    "    (void)option;\n"
                    "    return CURLE_OK;\n"
                    "}\n"
                    "\n"
                    "static inline CURLcode curl_easy_perform(CURL* curl) {\n"
                    "    (void)curl;\n"
                    "    return CURLE_OK;\n"
                    "}\n"
                    "\n"
                    "static inline CURLcode curl_easy_getinfo(CURL* curl, CURLINFO info, ...) {\n"
                    "    (void)curl;\n"
                    "    (void)info;\n"
                    "    return CURLE_OK;\n"
                    "}\n"
                    "\n"
                    "static inline const char* curl_easy_strerror(CURLcode code) {\n"
                    "    (void)code;\n"
                    "    return \"CURLE_OK\";\n"
                    "}\n"
                    "\n"
                    "#ifdef __cplusplus\n"
                    "}\n"
                    "#endif\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "boost/uuid/uuid.hpp" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <array>\n"
                    "#include <cstdint>\n"
                    "\n"
                    "namespace boost {\n"
                    "namespace uuids {\n"
                    "\n"
                    "struct uuid {\n"
                    "    std::array<uint8_t, 16> data;\n"
                    "    uuid() : data{} {}\n"
                    "};\n"
                    "\n"
                    "} // namespace uuids\n"
                    "} // namespace boost\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "boost/uuid/uuid_generators.hpp" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <boost/uuid/uuid.hpp>\n"
                    "\n"
                    "namespace boost {\n"
                    "namespace uuids {\n"
                    "\n"
                    "class random_generator {\n"
                    "public:\n"
                    "    uuid operator()() const {\n"
                    "        uuid id;\n"
                    "        id.data[0] = 0x12;\n"
                    "        id.data[1] = 0x34;\n"
                    "        id.data[2] = 0x56;\n"
                    "        id.data[3] = 0x78;\n"
                    "        return id;\n"
                    "    }\n"
                    "};\n"
                    "\n"
                    "} // namespace uuids\n"
                    "} // namespace boost\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "boost/uuid/uuid_io.hpp" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <string>\n"
                    "#include <boost/uuid/uuid.hpp>\n"
                    "\n"
                    "namespace boost {\n"
                    "namespace uuids {\n"
                    "\n"
                    "inline std::string to_string(const uuid&) {\n"
                    "    return \"00000000-0000-0000-0000-000000000000\";\n"
                    "}\n"
                    "\n"
                    "} // namespace uuids\n"
                    "} // namespace boost\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "boost/uuid/uuid.hpp" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <array>\n"
                    "#include <cstdint>\n"
                    "\n"
                    "namespace boost {\n"
                    "namespace uuids {\n"
                    "\n"
                    "struct uuid {\n"
                    "    std::array<uint8_t, 16> data;\n"
                    "    uuid() : data{} {}\n"
                    "};\n"
                    "\n"
                    "} // namespace uuids\n"
                    "} // namespace boost\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "boost/uuid/uuid_generators.hpp" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <boost/uuid/uuid.hpp>\n"
                    "\n"
                    "namespace boost {\n"
                    "namespace uuids {\n"
                    "\n"
                    "class random_generator {\n"
                    "public:\n"
                    "    uuid operator()() const {\n"
                    "        uuid id;\n"
                    "        id.data[0] = 0x12;\n"
                    "        id.data[1] = 0x34;\n"
                    "        id.data[2] = 0x56;\n"
                    "        id.data[3] = 0x78;\n"
                    "        return id;\n"
                    "    }\n"
                    "};\n"
                    "\n"
                    "} // namespace uuids\n"
                    "} // namespace boost\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "boost/uuid/uuid_io.hpp" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <string>\n"
                    "#include <boost/uuid/uuid.hpp>\n"
                    "\n"
                    "namespace boost {\n"
                    "namespace uuids {\n"
                    "\n"
                    "inline std::string to_string(const uuid&) {\n"
                    "    return \"00000000-0000-0000-0000-000000000000\";\n"
                    "}\n"
                    "\n"
                    "} // namespace uuids\n"
                    "} // namespace boost\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "rdkwindowmanager/include/rdkwindowmanagerevents.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <string>\n"
                    "\n"
                    "namespace RdkWindowManager {\n"
                    "\n"
                    "class RdkWindowManagerEventListener {\n"
                    "public:\n"
                    "    virtual ~RdkWindowManagerEventListener() = default;\n"
                    "    virtual void onUserInactive(double) {}\n"
                    "    virtual void onApplicationDisconnected(const std::string&) {}\n"
                    "    virtual void onReady(const std::string&) {}\n"
                    "    virtual void onApplicationConnected(const std::string&) {}\n"
                    "    virtual void onApplicationVisible(const std::string&) {}\n"
                    "    virtual void onApplicationHidden(const std::string&) {}\n"
                    "    virtual void onApplicationFocus(const std::string&) {}\n"
                    "    virtual void onApplicationBlur(const std::string&) {}\n"
                    "};\n"
                    "\n"
                    "} // namespace RdkWindowManager\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "rdkwindowmanager/include/rdkwindowmanager.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <cstdint>\n"
                    "#include <map>\n"
                    "#include <memory>\n"
                    "#include <semaphore.h>\n"
                    "#include <string>\n"
                    "#include <utility>\n"
                    "#include <vector>\n"
                    "#include <rdkwindowmanager/include/rdkwindowmanagerevents.h>\n"
                    "\n"
                    "namespace RdkWindowManager {\n"
                    "\n"
                    "class RdkWindowManagerData {\n"
                    "public:\n"
                    "    RdkWindowManagerData() = default;\n"
                    "    template <typename T>\n"
                    "    RdkWindowManagerData(const T&) {}\n"
                    "};\n"
                    "\n"
                    "inline double milliseconds() { return 0.0; }\n"
                    "inline void initialize() {}\n"
                    "inline void deinitialize() {}\n"
                    "inline void draw() {}\n"
                    "inline void update() {}\n"
                    "\n"
                    "class CompositorController {\n"
                    "public:\n"
                    "    static bool getClients(std::vector<std::string>&) { return true; }\n"
                    "    static void setEventListener(std::shared_ptr<RdkWindowManagerEventListener>) {}\n"
                    "    static bool createDisplay(const std::string&, const std::string&, uint32_t, uint32_t, bool, uint32_t, uint32_t, bool, bool, uint32_t, uint32_t) { return true; }\n"
                    "    static bool screenShot(uint8_t*&, uint32_t&) { return false; }\n"
                    "    static bool removeListener(const std::string&, std::shared_ptr<RdkWindowManagerEventListener>) { return true; }\n"
                    "    static bool addListener(const std::string&, std::shared_ptr<RdkWindowManagerEventListener>) { return true; }\n"
                    "    static bool enableKeyRepeats(bool) { return true; }\n"
                    "    static bool getKeyRepeatsEnabled(bool& enabled) { enabled = false; return true; }\n"
                    "    static bool ignoreKeyInputs(bool) { return true; }\n"
                    "    static bool enableInputEvents(const std::string&, bool) { return true; }\n"
                    "    static void setKeyRepeatConfig(bool, int, int) {}\n"
                    "    static bool setFocus(const std::string&) { return true; }\n"
                    "    static bool setVisibility(const std::string&, bool) { return true; }\n"
                    "    static bool getVisibility(const std::string&, bool& visible) { visible = true; return true; }\n"
                    "    static void enableInactivityReporting(bool) {}\n"
                    "    static bool addKeyIntercept(const std::string&, uint32_t, uint32_t, bool, bool) { return true; }\n"
                    "    static bool removeKeyIntercept(const std::string&, uint32_t, uint32_t) { return true; }\n"
                    "    static bool addKeyListener(const std::string&, uint32_t, uint32_t, const std::map<std::string, RdkWindowManagerData>&) { return true; }\n"
                    "    static bool addNativeKeyListener(const std::string&, uint32_t, uint32_t, const std::map<std::string, RdkWindowManagerData>&) { return true; }\n"
                    "    static bool removeKeyListener(const std::string&, uint32_t, uint32_t) { return true; }\n"
                    "    static bool removeNativeKeyListener(const std::string&, uint32_t, uint32_t) { return true; }\n"
                    "    static bool injectKey(uint32_t, uint32_t) { return true; }\n"
                    "    static bool generateKey(const std::string&, uint32_t, uint32_t, std::string&&) { return true; }\n"
                    "    static void setInactivityInterval(double) {}\n"
                    "    static void resetInactivityTime() {}\n"
                    "    static bool renderReady(const std::string&) { return true; }\n"
                    "    static bool enableDisplayRender(const std::string&, bool) { return true; }\n"
                    "    static bool getLastKeyPress(uint32_t& keyCode, uint32_t& modifiers, uint64_t& timestampInSeconds) { keyCode = 0; modifiers = 0; timestampInSeconds = 0; return true; }\n"
                    "    static bool setZorder(const std::string&, int32_t) { return true; }\n"
                    "    static bool getZOrder(const std::string&, int32_t& zOrder) { zOrder = 0; return true; }\n"
                    "    static bool startVncServer() { return true; }\n"
                    "    static bool stopVncServer() { return true; }\n"
                    "};\n"
                    "\n"
                    "} // namespace RdkWindowManager\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "rdkwindowmanager/include/linuxkeys.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#define RDK_WINDOW_MANAGER_FLAGS_CONTROL 0x01\n"
                    "#define RDK_WINDOW_MANAGER_FLAGS_SHIFT 0x02\n"
                    "#define RDK_WINDOW_MANAGER_FLAGS_ALT 0x04\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "UtilsString.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <cstdint>\n"
                    "#include <string>\n"
                    "\n"
                    "namespace Utils {\n"
                    "namespace String {\n"
                    "\n"
                    "inline bool imageEncoder(uint8_t*, uint32_t, bool, std::string& out) {\n"
                    "    out.clear();\n"
                    "    return true;\n"
                    "}\n"
                    "\n"
                    "} // namespace String\n"
                    "} // namespace Utils\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "telemetry_busmessage_sender.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "inline int t2_init(char*) { return 0; }\n"
                    "inline void t2_uninit() {}\n"
                    "inline int t2_event_s(char*, char*) { return 0; }\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "sys/prctl.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#ifndef PR_SET_NAME\n"
                    "#define PR_SET_NAME 0\n"
                    "#endif\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "IPackageImpl.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <map>\n"
                    "#include <memory>\n"
                    "#include <string>\n"
                    "#include <utility>\n"
                    "#include <vector>\n"
                    "\n"
                    "namespace packagemanager {\n"
                    "\n"
                    "using NameValue = std::pair<std::string, std::string>;\n"
                    "using NameValues = std::vector<NameValue>;\n"
                    "\n"
                    "enum class ApplicationType {\n"
                    "    SYSTEM = 0,\n"
                    "    INTERACTIVE = 1\n"
                    "};\n"
                    "\n"
                    "enum Result {\n"
                    "    SUCCESS = 0,\n"
                    "    FAILED = 1,\n"
                    "    VERSION_MISMATCH = 2,\n"
                    "    PERSISTENCE_FAILURE = 3,\n"
                    "    VERIFICATION_FAILURE = 4\n"
                    "};\n"
                    "\n"
                    "struct ConfigMetaData {\n"
                    "    bool dial{false};\n"
                    "    bool wanLanAccess{false};\n"
                    "    bool thunder{false};\n"
                    "    uint32_t systemMemoryLimit{0};\n"
                    "    uint32_t gpuMemoryLimit{0};\n"
                    "    std::string envVariables;\n"
                    "    std::vector<std::string> envVars;\n"
                    "    uint32_t userId{0};\n"
                    "    uint32_t groupId{0};\n"
                    "    uint32_t dataImageSize{0};\n"
                    "    std::vector<std::string> fkpsFiles;\n"
                    "    std::string capabilities;\n"
                    "    ApplicationType appType{ApplicationType::INTERACTIVE};\n"
                    "    std::string appPath;\n"
                    "    std::string command;\n"
                    "    std::string runtimePath;\n"
                    "    std::string ralfPkgPath;\n"
                    "    std::string runtimeType;\n"
                    "    std::string mimeType;\n"
                    "};\n"
                    "\n"
                    "using ConfigMetadataKey = std::pair<std::string, std::string>;\n"
                    "using ConfigMetadataArray = std::map<ConfigMetadataKey, ConfigMetaData>;\n"
                    "\n"
                    "class IPackageImpl {\n"
                    "public:\n"
                    "    static std::shared_ptr<IPackageImpl> instance() {\n"
                    "        return std::shared_ptr<IPackageImpl>(new IPackageImpl());\n"
                    "    }\n"
                    "\n"
                    "    Result Initialize(const std::string&, ConfigMetadataArray&) { return SUCCESS; }\n"
                    "    Result Install(const std::string&, const std::string&, const NameValues&, const std::string&, ConfigMetaData&) { return SUCCESS; }\n"
                    "    Result Uninstall(const std::string&) { return SUCCESS; }\n"
                    "    Result Lock(const std::string&, const std::string&, std::string&, ConfigMetaData&, NameValues&) { return SUCCESS; }\n"
                    "    Result Unlock(const std::string&, const std::string&) { return SUCCESS; }\n"
                    "    Result GetFileMetadata(const std::string&, std::string&, std::string&, ConfigMetaData&) { return SUCCESS; }\n"
                    "};\n"
                    "\n"
                    "} // namespace packagemanager\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            elif "IPackageImplDummy.h" == inc:
                header_path.write_text(
                    f"#ifndef {guard}\n"
                    f"#define {guard}\n"
                    "\n"
                    "#include <IPackageImpl.h>\n"
                    "\n"
                    "namespace packagemanager {\n"
                    "\n"
                    "class IPackageImplDummy : public IPackageImpl {\n"
                    "public:\n"
                    "    static std::shared_ptr<IPackageImplDummy> instance() {\n"
                    "        return std::shared_ptr<IPackageImplDummy>(new IPackageImplDummy());\n"
                    "    }\n"
                    "};\n"
                    "\n"
                    "} // namespace packagemanager\n"
                    "\n"
                    "#endif\n",
                    encoding="utf-8",
                )
            else:
                header_path.write_text(
                    f"#ifndef {guard}\n#define {guard}\n\n// Auto-generated fallback stub for {inc}\n\n#endif\n",
                    encoding="utf-8",
                )


def create_resolved_header_symlinks(stubs_dir: Path, missing_headers: Dict[str, str]) -> None:
    """Create symlinks for resolved headers so stubs/include does not shadow real headers."""
    include_root = stubs_dir / "include"
    include_root.mkdir(parents=True, exist_ok=True)

    for inc in sorted(missing_headers):
        resolved_from = missing_headers[inc]
        if resolved_from.startswith("<"):
            continue
        if should_skip_stub_generation(inc):
            continue

        header_path = include_root / inc
        header_path.parent.mkdir(parents=True, exist_ok=True)

        if header_path.is_symlink() or header_path.exists():
            try:
                header_path.unlink()
            except Exception:
                pass

        try:
            resolved_abs = Path(resolved_from).resolve()
            relative_path = os.path.relpath(resolved_abs, header_path.parent)
            os.symlink(relative_path, header_path)
        except Exception as e:
            print(f"Warning: Could not create symlink for {inc}: {e}")


def create_plugin_stubs(stubs_dir: Path) -> None:
    """Create comprehensive stubs for Thunder plugin headers that aren't available."""
    include_root = stubs_dir / "include"
    include_root.mkdir(parents=True, exist_ok=True)
    
    # Create plugins directory stub structure
    plugins_dir = include_root / "plugins"
    plugins_dir.mkdir(parents=True, exist_ok=True)
    
    iplugin_h = plugins_dir / "IPlugin.h"
    if iplugin_h.is_symlink():
        iplugin_h.unlink()
    iplugin_h.write_text(
        "#ifndef THUNDER_IPLUGIN_H\n"
        "#define THUNDER_IPLUGIN_H\n"
        "\n"
        "#include <string>\n"
        "#include <com/com.h>\n"
        "#include <core/core.h>\n"
        "\n"
        "namespace WPEFramework {\n"
        "namespace PluginHost {\n"
        "class IShell;\n"
        "class IPlugin : public Core::IUnknown {\n"
        "public:\n"
        "    virtual ~IPlugin() = default;\n"
        "    virtual const std::string Initialize(IShell*) { return std::string(); }\n"
        "    virtual void Deinitialize(IShell*) {}\n"
        "    virtual std::string Information() const { return std::string(); }\n"
        "};\n"
        "class IDispatcher {\n"
        "public:\n"
        "    virtual ~IDispatcher() = default;\n"
        "};\n"
        "class JSONRPC {\n"
        "public:\n"
        "    JSONRPC() = default;\n"
        "    virtual ~JSONRPC() = default;\n"
        "};\n"
        "} // namespace PluginHost\n"
        "} // namespace WPEFramework\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )

    isubsystem_h = plugins_dir / "ISubSystem.h"
    if isubsystem_h.is_symlink():
        isubsystem_h.unlink()
    isubsystem_h.write_text(
        "#ifndef THUNDER_ISUBSYSTEM_H\n"
        "#define THUNDER_ISUBSYSTEM_H\n"
        "\n"
        "#include <plugins/IPlugin.h>\n"
        "\n"
        "namespace WPEFramework {\n"
        "namespace PluginHost {\n"
        "class ISubSystem {\n"
        "public:\n"
        "    enum subsystem { INTERNET = 0, INSTALLATION = 1, NOT_INSTALLATION = 2, PLATFORM = 3, GRAPHICS = 4 };\n"
        "    virtual ~ISubSystem() = default;\n"
        "    virtual uint32_t Release() { return 1; }\n"
        "    virtual bool IsActive(const subsystem) const { return true; }\n"
        "    virtual void Set(const subsystem, void*) {}\n"
        "};\n"
        "} // namespace PluginHost\n"
        "} // namespace WPEFramework\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )

    ishell_h = plugins_dir / "IShell.h"
    if ishell_h.is_symlink():
        ishell_h.unlink()
    ishell_h.write_text(
        "#ifndef THUNDER_ISHELL_H\n"
        "#define THUNDER_ISHELL_H\n"
        "\n"
        "#include <plugins/IPlugin.h>\n"
        "#include <plugins/ISubSystem.h>\n"
        "\n"
        "namespace WPEFramework {\n"
        "namespace RPC { class IRemoteConnection; }\n"
        "namespace PluginHost {\n"
        "class IShell {\n"
        "public:\n"
        "    virtual ~IShell() = default;\n"
        "    enum state { DEACTIVATED = 0, ACTIVATED = 1, FAILURE = 2 };\n"
        "    class Job {\n"
        "    public:\n"
        "        static Job Create(IShell*, state, state) { return Job(); }\n"
        "    };\n"
        "    virtual uint32_t AddRef() { return 1; }\n"
        "    virtual uint32_t Release() { return 1; }\n"
        "    template <typename T>\n"
        "    T* Root(uint32_t&, uint32_t, const char*) { return nullptr; }\n"
        "    template <typename T>\n"
        "    T* QueryInterfaceByCallsign(const char*) { return nullptr; }\n"
        "    virtual std::string ConfigLine() const { return std::string(); }\n"
        "    virtual const std::string& Callsign() const { static std::string empty; return empty; }\n"
        "    virtual state State() const { return ACTIVATED; }\n"
        "    virtual ISubSystem* SubSystems() { static ISubSystem subsystem; return &subsystem; }\n"
        "    virtual void Register(RPC::IRemoteConnection::INotification*) {}\n"
        "    virtual void Unregister(RPC::IRemoteConnection::INotification*) {}\n"
        "    virtual RPC::IRemoteConnection* RemoteConnection(uint32_t) { return nullptr; }\n"
        "};\n"
        "} // namespace PluginHost\n"
        "} // namespace WPEFramework\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )

    istatecontrol_h = plugins_dir / "IStateControl.h"
    if istatecontrol_h.is_symlink():
        istatecontrol_h.unlink()
    istatecontrol_h.write_text(
        "#ifndef THUNDER_ISTATECONTROL_H\n"
        "#define THUNDER_ISTATECONTROL_H\n"
        "\n"
        "#include <plugins/IPlugin.h>\n"
        "\n"
        "namespace WPEFramework {\n"
        "namespace PluginHost {\n"
        "class IStateControl {\n"
        "public:\n"
        "    virtual ~IStateControl() = default;\n"
        "};\n"
        "} // namespace PluginHost\n"
        "} // namespace WPEFramework\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )

    plugins_h = plugins_dir / "plugins.h"
    if plugins_h.is_symlink():
        plugins_h.unlink()
    plugins_h.write_text(
        "#ifndef THUNDER_PLUGINS_H\n"
        "#define THUNDER_PLUGINS_H\n"
        "\n"
        "#include <plugins/IPlugin.h>\n"
        "#include <plugins/IShell.h>\n"
        "#include <plugins/ISubSystem.h>\n"
        "#include <plugins/IStateControl.h>\n"
        "\n"
        "#ifndef BEGIN_INTERFACE_MAP\n"
        "#define BEGIN_INTERFACE_MAP(x) public: uint32_t AddRef() { return 1; } uint32_t Release() { return 1; }\n"
        "#endif\n"
        "#ifndef INTERFACE_ENTRY\n"
        "#define INTERFACE_ENTRY(x)\n"
        "#endif\n"
        "#ifndef INTERFACE_AGGREGATE\n"
        "#define INTERFACE_AGGREGATE(x, y)\n"
        "#endif\n"
        "#ifndef END_INTERFACE_MAP\n"
        "#define END_INTERFACE_MAP\n"
        "#endif\n"
            "#ifndef SERVICE_REGISTRATION\n"
            "#define SERVICE_REGISTRATION(...)\n"
        "#endif\n"
            "#ifndef ASSERT\n"
            "#define ASSERT(x) do { (void)(x); } while (0)\n"
            "#endif\n"
            "#ifndef VARIABLE_IS_NOT_USED\n"
            "#define VARIABLE_IS_NOT_USED\n"
            "#endif\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )

    tracing_dir = include_root / "tracing"
    tracing_dir.mkdir(parents=True, exist_ok=True)
    tracing_h = tracing_dir / "tracing.h"
    if tracing_h.is_symlink():
        tracing_h.unlink()
    tracing_h.write_text(
        "#ifndef TRACING_TRACING_H\n"
        "#define TRACING_TRACING_H\n"
        "\n"
        "#define TRACE_GLOBAL(cat, fmt, ...)\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )

    tracing_logging_h = tracing_dir / "Logging.h"
    if tracing_logging_h.is_symlink():
        tracing_logging_h.unlink()
    tracing_logging_h.write_text(
        "#ifndef TRACING_LOGGING_H\n"
        "#define TRACING_LOGGING_H\n"
        "\n"
        "#include <stdio.h>\n"
        "\n"
        "namespace WPEFramework {\n"
        "namespace Logging {\n"
        "enum level { Startup = 0, Shutdown = 1, Notification = 2 };\n"
        "}\n"
        "}\n"
        "\n"
        "#ifndef SYSLOG\n"
        "#define SYSLOG(level, args)\n"
        "#endif\n"
        "#ifndef LOGINFO\n"
        "#define LOGINFO(fmt, ...) do { fprintf(stderr, \"INFO: \" fmt \"\\n\", ##__VA_ARGS__); } while (0)\n"
        "#endif\n"
        "#ifndef LOGWARN\n"
        "#define LOGWARN(fmt, ...) do { fprintf(stderr, \"WARN: \" fmt \"\\n\", ##__VA_ARGS__); } while (0)\n"
        "#endif\n"
        "#ifndef LOGERR\n"
        "#define LOGERR(fmt, ...) do { fprintf(stderr, \"ERR: \" fmt \"\\n\", ##__VA_ARGS__); } while (0)\n"
        "#endif\n"
        "#ifndef ASSERT\n"
        "#define ASSERT(x) do { (void)(x); } while (0)\n"
        "#endif\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )
    
    # Also create a core/core.h stub (used by Module.h)
    core_dir = include_root / "core"
    core_dir.mkdir(parents=True, exist_ok=True)
    core_h = core_dir / "core.h"
    if core_h.is_symlink():
        core_h.unlink()
    core_h.write_text(
            "#ifndef CORE_CORE_H\n"
            "#define CORE_CORE_H\n"
            "\n"
            "#include <stdint.h>\n"
            "#include <string>\n"
            "#include <memory>\n"
            "#include <list>\n"
            "#include <map>\n"
            "#include <vector>\n"
            "#include <queue>\n"
            "#include <algorithm>\n"
            "#include <functional>\n"
            "#include <cmath>\n"
            "#include <iostream>\n"
            "#include <chrono>\n"
            "#include <cstdlib>\n"
            "#include <cstring>\n"
            "#include <dirent.h>\n"
            "#include <sys/stat.h>\n"
            "#include <com/com.h>\n"
            "\n"
            "#ifndef EXTERNAL\n"
            "#define EXTERNAL\n"
            "#endif\n"
            "#ifndef _T\n"
            "#define _T(x) x\n"
            "#endif\n"
            "\n"
            "namespace WPEFramework {\n"
            "using string = std::string;\n"
            "namespace Core {\n"
            "using hresult = uint32_t;\n"
            "static constexpr hresult ERROR_NONE = 0;\n"
            "static constexpr hresult ERROR_GENERAL = 1;\n"
            "static constexpr hresult ERROR_UNAVAILABLE = 2;\n"
            "static constexpr hresult ERROR_INVALID_PARAMETER = 3;\n"
            "static constexpr hresult ERROR_UNKNOWN_KEY = 4;\n"
            "static constexpr hresult ERROR_BAD_REQUEST = 5;\n"
            "static constexpr hresult ERROR_ILLEGAL_STATE = 6;\n"
            "static constexpr hresult ERROR_INVALID_SIGNATURE = 7;\n"
            "static constexpr hresult ERROR_TIMEDOUT = 8;\n"
            "inline const char* FileNameOnly(const char* file) { return (nullptr != file) ? file : \"\"; }\n"
            "class IUnknown {\n"
            "public:\n"
            "    virtual ~IUnknown() = default;\n"
            "    virtual uint32_t AddRef() { return 1; }\n"
            "    virtual uint32_t Release() { return 1; }\n"
            "};\n"
            "class CriticalSection {\n"
            "public:\n"
            "    void Lock() {}\n"
            "    void Unlock() {}\n"
            "};\n"
            "template <typename LOCK>\n"
            "class SafeSyncType {\n"
            "public:\n"
            "    explicit SafeSyncType(LOCK& lock): _lock(lock) { _lock.Lock(); }\n"
            "    ~SafeSyncType() { _lock.Unlock(); }\n"
            "private:\n"
            "    LOCK& _lock;\n"
            "};\n"
            "template <typename T>\n"
            "class Sink : public T {\n"
            "public:\n"
            "    template <typename... Args>\n"
            "    explicit Sink(Args&&... args): T(args...) {}\n"
            "};\n"
            "class IDispatch {\n"
            "public:\n"
            "    virtual ~IDispatch() = default;\n"
            "    virtual void Dispatch() {}\n"
            "};\n"
            "template <typename T>\n"
            "class ProxyType {\n"
            "public:\n"
            "    ProxyType() = default;\n"
            "    explicit ProxyType(T* p): _p(p) {}\n"
            "    static ProxyType<T> Create() { return ProxyType<T>(); }\n"
            "    template <typename... Args>\n"
            "    static ProxyType<T> Create(Args&&...) { return ProxyType<T>(); }\n"
            "    T* operator->() { return _p; }\n"
            "    const T* operator->() const { return _p; }\n"
            "    operator bool() const { return nullptr != _p; }\n"
            "private:\n"
            "    T* _p{nullptr};\n"
            "};\n"
            "template <typename T, typename U>\n"
            "inline ProxyType<T> proxy_cast(const ProxyType<U>&) { return ProxyType<T>(); }\n"
            "template <typename T>\n"
            "class Service {\n"
            "public:\n"
            "    template <typename INTERFACE, typename... Args>\n"
            "    static INTERFACE* Create(Args&&...) { return nullptr; }\n"
            "};\n"
            "class IWorkerPool {\n"
            "public:\n"
            "    static IWorkerPool& Instance() { static IWorkerPool p; return p; }\n"
            "    template <typename T>\n"
            "    void Submit(const T&) {}\n"
            "};\n"
            "class SystemInfo {\n"
            "public:\n"
            "    static void SetEnvironment(const char* key, const char* value) {\n"
            "        if ((nullptr != key) && (nullptr != value)) {\n"
            "            (void)setenv(key, value, 1);\n"
            "        }\n"
            "    }\n"
            "};\n"
            "namespace JSON {\n"
            "class Container {\n"
            "public:\n"
            "    virtual ~Container() = default;\n"
            "    template <typename T>\n"
            "    void Add(const char*, T*) {}\n"
            "    bool FromString(const std::string&) { return true; }\n"
            "};\n"
            "class String {\n"
            "public:\n"
            "    String() : _value(), _isSet(false) {}\n"
            "    const std::string& Value() const { return _value; }\n"
            "    void Value(const std::string& value) { _value = value; _isSet = true; }\n"
            "    bool IsSet() const { return _isSet; }\n"
            "    operator std::string() const { return _value; }\n"
            "private:\n"
            "    std::string _value;\n"
            "    bool _isSet;\n"
            "};\n"
            "class DecUInt32 {\n"
            "public:\n"
            "    DecUInt32() : _value(0), _isSet(false) {}\n"
            "    uint32_t Value() const { return _value; }\n"
            "    void Value(const uint32_t value) { _value = value; _isSet = true; }\n"
            "    bool IsSet() const { return _isSet; }\n"
            "private:\n"
            "    uint32_t _value;\n"
            "    bool _isSet;\n"
            "};\n"
            "} // namespace JSON\n"
            "} // namespace Core\n"
            "class JsonArray;\n"
            "class JsonObject;\n"
            "class JsonValue {\n"
            "public:\n"
            "    JsonValue() = default;\n"
            "    JsonValue(const JsonObject&) {}\n"
            "    JsonValue(const std::string&) {}\n"
            "    JsonValue(const char*) {}\n"
            "    JsonValue(double) {}\n"
            "    JsonValue(bool) {}\n"
            "    struct type {\n"
            "        enum Enum { ARRAY = 0, STRING = 1, OBJECT = 2, NUMBER = 3, BOOLEAN = 4, UNKNOWN = 5 };\n"
            "    };\n"
            "    JsonValue& operator=(const std::string&) { return *this; }\n"
            "    JsonValue& operator=(const char*) { return *this; }\n"
            "    JsonValue& operator=(int) { return *this; }\n"
            "    JsonValue& operator=(uint32_t) { return *this; }\n"
            "    JsonValue& operator=(double) { return *this; }\n"
            "    JsonValue& operator=(const JsonObject&) { return *this; }\n"
            "    bool FromString(const std::string&) { return true; }\n"
            "    type::Enum Content() const { return type::ARRAY; }\n"
            "    JsonArray Array() const;\n"
            "    JsonObject Object() const;\n"
            "    std::string String() const { return std::string(); }\n"
            "    int Number() const { return 0; }\n"
            "    double Double() const { return 0.0; }\n"
            "    bool Boolean() const { return false; }\n"
            "    operator std::string() const { return String(); }\n"
            "};\n"
            "class JsonObject {\n"
            "public:\n"
            "    bool HasLabel(const char*) const { return false; }\n"
            "    bool FromString(const std::string&) { return true; }\n"
            "    JsonValue& operator[](const char*) { static JsonValue value; return value; }\n"
            "    JsonValue operator[](const char*) const { return JsonValue(); }\n"
            "    bool ToString(std::string& out) const { out = \"{}\"; return true; }\n"
            "};\n"
            "class JsonArrayItem {\n"
            "public:\n"
            "    JsonValue::type::Enum Content() const { return JsonValue::type::UNKNOWN; }\n"
            "    JsonObject Object() const { return JsonObject(); }\n"
            "    std::string String() const { return std::string(); }\n"
            "};\n"
            "class JsonArray {\n"
            "public:\n"
            "    void Add(const JsonObject&) {}\n"
            "    void Add(const std::string&) {}\n"
            "    void Add(const char*) {}\n"
            "    bool IsSet() const { return false; }\n"
            "    bool FromString(const std::string&) { return true; }\n"
            "    size_t Length() const { return 0; }\n"
            "    JsonArrayItem operator[](size_t) const { return JsonArrayItem(); }\n"
            "    bool ToString(std::string& out) const { out = \"[]\"; return true; }\n"
            "};\n"
            "inline JsonArray JsonValue::Array() const { return JsonArray(); }\n"
            "inline JsonObject JsonValue::Object() const { return JsonObject(); }\n"
            "} // namespace WPEFramework\n"
            "\n"
            "#endif\n",
        encoding="utf-8",
    )

    semaphore_h = include_root / "semaphore"
    if semaphore_h.is_symlink():
        semaphore_h.unlink()
    semaphore_h.write_text(
        "#ifndef FUZZ_STUB_SEMAPHORE_HEADER\n"
        "#define FUZZ_STUB_SEMAPHORE_HEADER\n"
        "\n"
        "// On Linux use the real POSIX semaphore header to avoid sem_t redefinition conflicts\n"
        "#if defined(__linux__)\n"
        "#include <semaphore.h>\n"
        "#else\n"
        "typedef int sem_t;\n"
        "\n"
        "static inline int sem_init(sem_t* s, int pshared, unsigned int value) {\n"
        "    (void)pshared;\n"
        "    if (s) { *s = (int)value; }\n"
        "    return 0;\n"
        "}\n"
        "\n"
        "static inline int sem_destroy(sem_t* s) {\n"
        "    (void)s;\n"
        "    return 0;\n"
        "}\n"
        "\n"
        "static inline int sem_post(sem_t* s) {\n"
        "    if (s) { ++(*s); }\n"
        "    return 0;\n"
        "}\n"
        "\n"
        "static inline int sem_wait(sem_t* s) {\n"
        "    if (s && (*s) > 0) { --(*s); }\n"
        "    return 0;\n"
        "}\n"
        "#endif\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )

    sys_dir = include_root / "sys"
    sys_dir.mkdir(parents=True, exist_ok=True)
    prctl_h = sys_dir / "prctl.h"
    if prctl_h.is_symlink():
        prctl_h.unlink()
    prctl_h.write_text(
        "#ifndef SYS_PRCTL_H_FUZZ_STUB\n"
        "#define SYS_PRCTL_H_FUZZ_STUB\n"
        "\n"
        "#ifndef PR_SET_NAME\n"
        "#define PR_SET_NAME 0\n"
        "#endif\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )

    telemetry_sender_h = include_root / "telemetry_busmessage_sender.h"
    if telemetry_sender_h.is_symlink():
        telemetry_sender_h.unlink()
    telemetry_sender_h.write_text(
        "#ifndef TELEMETRY_BUSMESSAGE_SENDER_H_FUZZ_STUB\n"
        "#define TELEMETRY_BUSMESSAGE_SENDER_H_FUZZ_STUB\n"
        "\n"
        "inline int t2_init(char*) { return 0; }\n"
        "inline void t2_uninit() {}\n"
        "inline int t2_event_s(char*, char*) { return 0; }\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )

    # Create COM-RPC shim for Exchange::Ids dependency.
    com_dir = include_root / "com"
    com_dir.mkdir(parents=True, exist_ok=True)
    com_h = com_dir / "com.h"
    if com_h.is_symlink():
        com_h.unlink()
    com_h.write_text(
            "#ifndef COM_COM_H_FUZZ_STUB_H\n"
            "#define COM_COM_H_FUZZ_STUB_H\n"
            "\n"
            "#include <stdint.h>\n"
            "\n"
            "namespace WPEFramework {\n"
            "namespace RPC {\n"
            "namespace IDS {\n"
            "static constexpr uint32_t ID_EXTERNAL_CC_INTERFACE_OFFSET = 0x10000;\n"
            "}\n"
            "static constexpr uint32_t ID_STRINGITERATOR = 0x10001;\n"
            "static constexpr uint32_t ID_VALUEITERATOR = 0x10002;\n"
            "template <typename T, uint32_t ID>\n"
            "class IIteratorType {\n"
            "public:\n"
            "    virtual ~IIteratorType() = default;\n"
            "    virtual bool Next(T&) { return false; }\n"
            "    virtual uint32_t Release() { return 1; }\n"
            "};\n"
            "template <typename INTERFACE>\n"
            "class IteratorType {};\n"
            "template <typename T, uint32_t ID>\n"
            "using IteratorTypeImpl = IIteratorType<T, ID>;\n"
            "using IStringIterator = IIteratorType<std::string, ID_STRINGITERATOR>;\n"
            "using IValueIterator = IIteratorType<uint32_t, ID_VALUEITERATOR>;\n"
            "class StringIterator : public IStringIterator {};\n"
            "class ValueIterator : public IValueIterator {};\n"
            "class IRemoteConnection {\n"
            "public:\n"
            "    class INotification {\n"
            "    public:\n"
            "        virtual ~INotification() = default;\n"
            "        virtual void Activated(IRemoteConnection*) {}\n"
            "        virtual void Deactivated(IRemoteConnection*) {}\n"
            "    };\n"
            "    uint32_t Id() const { return 0; }\n"
            "    void Terminate() {}\n"
            "    uint32_t Release() { return 1; }\n"
            "};\n"
            "} // namespace RPC\n"
            "} // namespace WPEFramework\n"
            "\n"
            "#endif\n",
        encoding="utf-8",
    )

    # macOS shim for Linux-style syscall.h include in helpers/UtilsLogging.h
    syscall_h = include_root / "syscall.h"
    if syscall_h.is_symlink():
        syscall_h.unlink()
    syscall_h.write_text(
        "#ifndef SYSCALL_H_FUZZ_STUB_H\n"
        "#define SYSCALL_H_FUZZ_STUB_H\n"
        "\n"
        "#include <sys/types.h>\n"
        "#include <unistd.h>\n"
        "\n"
        "#ifndef __NR_gettid\n"
        "#define __NR_gettid 0\n"
        "#endif\n"
        "#ifndef SYS_gettid\n"
        "#define SYS_gettid __NR_gettid\n"
        "#endif\n"
        "#if defined(__APPLE__)\n"
        "#ifndef syscall\n"
        "#define syscall(...) ((pid_t)getpid())\n"
        "#endif\n"
        "#endif\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )

    interfaces_json_dir = include_root / "interfaces" / "json"
    interfaces_json_dir.mkdir(parents=True, exist_ok=True)
    japp_h = interfaces_json_dir / "JAppManager.h"
    if japp_h.is_symlink():
        japp_h.unlink()
    japp_h.write_text(
        "#ifndef INTERFACES_JSON_JAPPMANAGER_H\n"
        "#define INTERFACES_JSON_JAPPMANAGER_H\n"
        "\n"
        "namespace WPEFramework {\n"
        "namespace Exchange {\n"
        "namespace JAppManager {\n"
        "struct Event {\n"
        "    template <typename... Args> static void OnAppInstalled(Args...) {}\n"
        "    template <typename... Args> static void OnAppUninstalled(Args...) {}\n"
        "    template <typename... Args> static void OnAppLifecycleStateChanged(Args...) {}\n"
        "    template <typename... Args> static void OnAppLaunchRequest(Args...) {}\n"
        "    template <typename... Args> static void OnAppUnloaded(Args...) {}\n"
        "};\n"
        "template <typename... Args> inline void Register(Args...) {}\n"
        "template <typename... Args> inline void Unregister(Args...) {}\n"
        "} // namespace JAppManager\n"
        "} // namespace Exchange\n"
        "} // namespace WPEFramework\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )

    jdata_h = interfaces_json_dir / "JsonData_AppManager.h"
    if jdata_h.is_symlink():
        jdata_h.unlink()
    jdata_h.write_text(
        "#ifndef INTERFACES_JSON_JSONDATA_APPMANAGER_H\n"
        "#define INTERFACES_JSON_JSONDATA_APPMANAGER_H\n"
        "#endif\n",
        encoding="utf-8",
    )

    telemetry_base_h = include_root / "TelemetryReportingBase.h"
    if telemetry_base_h.is_symlink():
        telemetry_base_h.unlink()
    telemetry_base_h.write_text(
        "#ifndef TELEMETRY_REPORTING_BASE_H\n"
        "#define TELEMETRY_REPORTING_BASE_H\n"
        "\n"
        "#include <stdint.h>\n"
        "#include <time.h>\n"
        "#include <string>\n"
        "#include <core/core.h>\n"
        "#include <TelemetryMarkers.h>\n"
        "\n"
        "namespace WPEFramework { namespace PluginHost { class IShell; } }\n"
        "\n"
        "namespace Utils {\n"
        "inline bool isTelemetryMetricsEnabled() { return true; }\n"
        "\n"
        "class TelemetryClient {\n"
        "public:\n"
        "    void record(const std::string&, const std::string&, const std::string&) {}\n"
        "    void publish(const std::string&, const std::string&) {}\n"
        "};\n"
        "\n"
        "class TelemetryReportingBase {\n"
        "public:\n"
        "    virtual ~TelemetryReportingBase() = default;\n"
        "    virtual void initialize(WPEFramework::PluginHost::IShell* service) { setService(service); }\n"
        "    virtual time_t getCurrentTimestampMs() { return 0; }\n"
        "\n"
        "protected:\n"
        "    void setService(WPEFramework::PluginHost::IShell*) {}\n"
        "    uint32_t initializeTelemetryClient() { return 0; }\n"
        "    void resetTelemetryClient() {}\n"
        "    bool ensureTelemetryClient() { return true; }\n"
        "    bool isTelemetryClientAvailable() const { return true; }\n"
        "    time_t currentTimestampMs() const { return 0; }\n"
        "    int durationSinceMs(uint64_t) const { return 0; }\n"
        "    uint32_t recordTelemetry(const std::string&, const WPEFramework::JsonObject&, const std::string&) { return 0; }\n"
        "    uint32_t recordAndPublishTelemetry(const std::string&, const WPEFramework::JsonObject&, const std::string&, bool) { return 0; }\n"
        "    TelemetryClient& getTelemetryClient() { static TelemetryClient client; return client; }\n"
        "};\n"
        "}\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )

    telemetry_markers_h = include_root / "TelemetryMarkers.h"
    if telemetry_markers_h.is_symlink():
        telemetry_markers_h.unlink()
    telemetry_markers_h.write_text(
        "#ifndef TELEMETRY_MARKERS_H\n"
        "#define TELEMETRY_MARKERS_H\n"
        "\n"
        "#define TELEMETRY_MARKER_LAUNCH_TIME \"TELEMETRY_MARKER_LAUNCH_TIME\"\n"
        "#define TELEMETRY_MARKER_CLOSE_TIME \"TELEMETRY_MARKER_CLOSE_TIME\"\n"
        "#define TELEMETRY_MARKER_LAUNCH_ERROR \"TELEMETRY_MARKER_LAUNCH_ERROR\"\n"
        "#define TELEMETRY_MARKER_CLOSE_ERROR \"TELEMETRY_MARKER_CLOSE_ERROR\"\n"
        "#define TELEMETRY_MARKER_APP_CRASHED \"TELEMETRY_MARKER_APP_CRASHED\"\n"
        "#define TELEMETRY_MARKER_DOWNLOAD_TIME \"TELEMETRY_MARKER_DOWNLOAD_TIME\"\n"
        "#define TELEMETRY_MARKER_DOWNLOAD_ERROR \"TELEMETRY_MARKER_DOWNLOAD_ERROR\"\n"
        "#define TELEMETRY_MARKER_INSTALL_TIME \"TELEMETRY_MARKER_INSTALL_TIME\"\n"
        "#define TELEMETRY_MARKER_INSTALL_ERROR \"TELEMETRY_MARKER_INSTALL_ERROR\"\n"
        "#define TELEMETRY_MARKER_UNINSTALL_TIME \"TELEMETRY_MARKER_UNINSTALL_TIME\"\n"
        "#define TELEMETRY_MARKER_UNINSTALL_ERROR \"TELEMETRY_MARKER_UNINSTALL_ERROR\"\n"
        "#define TELEMETRY_MARKER_SUSPEND_TIME \"TELEMETRY_MARKER_SUSPEND_TIME\"\n"
        "#define TELEMETRY_MARKER_RESUME_TIME \"TELEMETRY_MARKER_RESUME_TIME\"\n"
        "#define TELEMETRY_MARKER_HIBERNATE_TIME \"TELEMETRY_MARKER_HIBERNATE_TIME\"\n"
        "#define TELEMETRY_MARKER_WAKE_TIME \"TELEMETRY_MARKER_WAKE_TIME\"\n"
        "#define TELEMETRY_MARKER_PREFIX \"TELEMETRY_MARKER_PREFIX\"\n"
        "\n"
        "#endif\n",
        encoding="utf-8",
    )


def create_interface_header_symlinks(stubs_dir: Path, missing_headers: Dict[str, str], dep_root: Path) -> None:
    """Create symlinks for interface headers that map to actual API headers in dependencies."""
    include_root = stubs_dir / "include"
    include_root.mkdir(parents=True, exist_ok=True)
    
    for inc in sorted(missing_headers):
        resolved_from = missing_headers[inc]
        # Create symlinks only for resolved headers (not stubs)
        if not resolved_from.startswith("<"):
            if "interfaces/Ids.h" == inc:
                # Wrapper ensures RPC namespace is visible before dependency Ids enum uses RPC::IDS.
                header_path = include_root / inc
                header_path.parent.mkdir(parents=True, exist_ok=True)
                if header_path.is_symlink() or header_path.exists():
                    try:
                        header_path.unlink()
                    except Exception:
                        pass
                header_path.write_text(
                    "#pragma once\n"
                    "#include <com/com.h>\n"
                    f"#include \"{Path(resolved_from).resolve()}\"\n",
                    encoding="utf-8",
                )
                continue
            header_path = include_root / inc
            header_path.parent.mkdir(parents=True, exist_ok=True)
            
            # Remove existing symlink if present
            if header_path.is_symlink() or header_path.exists():
                try:
                    header_path.unlink()
                except Exception:
                    pass
            
            # Create symlink to resolved header
            try:
                resolved_abs = Path(resolved_from).resolve()
                relative_path = os.path.relpath(resolved_abs, header_path.parent)
                os.symlink(relative_path, header_path)
            except Exception as e:
                print(f"Warning: Could not create symlink for {inc}: {e}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate fallback stubs")
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--dependencies-root", required=True)
    parser.add_argument("--scope", default=",".join(DEFAULT_SCOPE_FOLDERS))
    parser.add_argument("--out-dir", default="fuzz/stats/auto/stubs")
    parser.add_argument("--manifest", default="fuzz/stats/results/stub_manifest.json")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    dep_root = Path(args.dependencies_root).resolve()
    include_scope = parse_scope(args.scope)
    out_dir = (repo_root / args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    includes = collect_includes(repo_root, include_scope)
    missing = resolve_missing_headers(repo_root, dep_root, includes, include_scope)
    write_profile_stubs(out_dir)
    purge_collected_include_entries(out_dir, includes)
    create_plugin_stubs(out_dir)
    write_missing_header_stubs(out_dir, missing)
    create_resolved_header_symlinks(out_dir, missing)
    create_interface_header_symlinks(out_dir, missing, dep_root)

    payload = {
        "dependencies_root": str(dep_root),
        "missing_headers": [{"header": h, "resolved_from": missing[h]} for h in sorted(missing)],
        "failure_profiles": [
            "VALID_RETURN",
            "NULL_RETURN",
            "ALLOCATION_FAILURE",
            "TIMEOUT",
            "INVALID_STATE",
            "MALFORMED_RESPONSE",
            "RPC_FAILURE",
        ],
    }
    manifest = (repo_root / args.manifest).resolve()
    manifest.parent.mkdir(parents=True, exist_ok=True)
    manifest.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
