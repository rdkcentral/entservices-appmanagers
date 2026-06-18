#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
import re
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Dict, List, Tuple

SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_DISPATCH_MAPPING_FILE = SCRIPT_DIR / "dispatch_mapping.json"

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

FUNCTION_RE = re.compile(
    r"^\s*(?P<prefix>(?:static\s+|inline\s+|virtual\s+|constexpr\s+|friend\s+|extern\s+|\w+::)*)"
    r"(?P<ret>[~\w:\<\>\*&\s]+?)\s+"
    r"(?P<name>[A-Za-z_][\w:]*)\s*\((?P<args>[^;{}]*)\)\s*"
    r"(?P<suffix>(?:const\s*)?(?:override\s*)?(?:final\s*)?(?:noexcept\s*)?)"
    r"(\{|$)"
)

CALLABLE_LIKE_RE = re.compile(r"\b([A-Za-z_][\w:]*)\s*\([^;]*\)\s*\{")

DISALLOWED_NAMES = {
    "if",
    "for",
    "while",
    "switch",
    "return",
    "else",
    "do",
    "case",
    "BEGIN_INTERFACE_MAP",
    "INTERFACE_ENTRY",
    "END_INTERFACE_MAP",
}

DISALLOWED_RET_TOKENS = {
    "if",
    "for",
    "while",
    "switch",
    "return",
    "else",
    "do",
    "case",
}

DISPATCH_SUPPORTED_TARGETS = {
    "AppManagerImplementation::LaunchApp",
    "AppManagerImplementation::PreloadApp",
    "AppManagerImplementation::CloseApp",
    "AppManagerImplementation::TerminateApp",
    "AppManagerImplementation::KillApp",
    "AppManagerImplementation::SendIntent",
    "AppManagerImplementation::GetInstalledApps",
    "AppManagerImplementation::GetLoadedApps",
    "LifecycleInterfaceConnector::getLoadedApps",
    "AppManagerImplementation::IsInstalled",
    "AppManagerImplementation::GetAppProperty",
    "AppManagerImplementation::SetAppProperty",
    "AppManagerImplementation::GetAppMetadata",
    "AppManagerImplementation::StartSystemApp",
    "AppManagerImplementation::StopSystemApp",
    "AppManagerImplementation::ClearAppData",
    "AppManagerImplementation::ClearAllAppData",
    "AppManagerImplementation::GetMaxRunningApps",
    "AppManagerImplementation::GetMaxHibernatedApps",
    "AppManagerImplementation::GetMaxHibernatedFlashUsage",
    "AppManagerImplementation::GetMaxInactiveRamUsage",
    "AppManager::Notification::OnAppInstalled",
    "Notification::OnAppInstalled",
    "OnAppInstalled",
    "AppManager::Notification::OnAppUninstalled",
    "Notification::OnAppUninstalled",
    "OnAppUninstalled",
    "AppManagerImplementation::AddRef",
    "AppManagerImplementation::Release",
    "StorageManagerImplementation::CreateStorage",
    "StorageManagerImplementation::GetStorage",
    "StorageManagerImplementation::DeleteStorage",
    "StorageManagerImplementation::Clear",
    "StorageManagerImplementation::ClearAll",
    "StorageManagerImplementation::Configure",
    "RequestHandler::SetBaseStoragePath",
    "RequestHandler::setCurrentService",
    "RequestHandler::CreateStorage",
    "RequestHandler::GetStorage",
    "RequestHandler::DeleteStorage",
    "RequestHandler::Clear",
    "RequestHandler::ClearAll",
    "RequestHandler::populateAppInfoCacheFromStoragePath",
    "RequestHandler::createPersistentStoreRemoteStoreObject",
    "RequestHandler::releasePersistentStoreRemoteStoreObject",
    "RequestHandler::isValidAppStorageDirectory",
    "RequestHandler::appQuotaSizeProperty",
    "AppStorageManagerTelemetryReporting::initialize",
    "AppStorageManagerTelemetryReporting::reset",
    "AppStorageManagerTelemetryReporting::recordGetStorageTelemetry",
    "DownloadManagerImplementation::Initialize",
    "DownloadManagerImplementation::Deinitialize",
    "DownloadManagerImplementation::Pause",
    "DownloadManagerImplementation::Resume",
    "DownloadManagerImplementation::Cancel",
    "DownloadManagerImplementation::Delete",
    "DownloadManagerImplementation::Progress",
    "DownloadManagerImplementation::RateLimit",
    "LifecycleManagerImplementation::GetLoadedApps",
    "LifecycleManagerImplementation::IsAppLoaded",
    "LifecycleManagerImplementation::UnloadApp",
    "LifecycleManagerImplementation::KillApp",
    "LifecycleManagerImplementation::SendIntentToActiveApp",
    "LifecycleManagerImplementation::AppReady",
    "LifecycleManagerImplementation::StateChangeComplete",
    "LifecycleManagerImplementation::CloseApp",
    "PackageManagerImplementation::Initialize",
    "PackageManagerImplementation::Deinitialize",
    "PackageManagerImplementation::Pause",
    "PackageManagerImplementation::Resume",
    "PackageManagerImplementation::Cancel",
    "PackageManagerImplementation::Delete",
    "PackageManagerImplementation::GetStorageInformation",
    "PackageManagerImplementation::RateLimit",
    "PackageManagerImplementation::Uninstall",
    "PreinstallManagerImplementation::StartPreinstall",
    "PreinstallManagerImplementation::GetPreinstallState",
    "PreinstallManagerImplementation::Configure",
    "RDKWindowManagerImplementation::Initialize",
    "RDKWindowManagerImplementation::Deinitialize",
    "RDKWindowManagerImplementation::GetApps",
    "RDKWindowManagerImplementation::SetFocus",
    "RDKWindowManagerImplementation::SetVisible",
    "RDKWindowManagerImplementation::GetVisibility",
    "RDKWindowManagerImplementation::EnableKeyRepeats",
    "RDKWindowManagerImplementation::GetKeyRepeatsEnabled",
    "RDKWindowManagerImplementation::ResetInactivityTime",
    "RDKWindowManagerImplementation::CreateDisplay",
    "TelemetryMetricsImplementation::Record",
    "TelemetryMetricsImplementation::Publish",
}


def load_dispatch_supported_targets(mapping_file: Path) -> set:
    if not mapping_file.exists():
        return set(DISPATCH_SUPPORTED_TARGETS)

    try:
        payload = json.loads(mapping_file.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return set(DISPATCH_SUPPORTED_TARGETS)

    items = payload.get("supported_targets", [])
    if not isinstance(items, list):
        return set(DISPATCH_SUPPORTED_TARGETS)

    loaded = set()
    for item in items:
        if isinstance(item, str) and item.strip():
            loaded.add(item.strip())

    if not loaded:
        return set(DISPATCH_SUPPORTED_TARGETS)

    return loaded


DISPATCH_SUPPORTED_TARGETS = load_dispatch_supported_targets(DEFAULT_DISPATCH_MAPPING_FILE)


def is_probable_function(line: str, ret: str, name: str, args: str) -> bool:
    stripped = line.strip()
    if not stripped or stripped.startswith("#"):
        return False
    if name in DISALLOWED_NAMES:
        return False
    if ret.strip() in DISALLOWED_RET_TOKENS:
        return False
    if stripped.startswith(("if ", "if(", "for ", "for(", "while ", "while(", "switch ", "switch(", "return ")):
        return False
    if re.match(r"^\s*else\s+if\s*\(", line):
        return False
    if re.match(r"^\s*[,:]\s*[A-Za-z_][\w:]*\s*\(", line):
        return False
    if "==" in args or "&&" in args or "||" in args:
        return False
    return True


def is_dispatch_supported(function_name: str) -> bool:
    return function_name in DISPATCH_SUPPORTED_TARGETS


def unmapped_reason(function_name: str) -> str:
    if "::" not in function_name:
        return "non_scoped_or_macro_like"

    short_name = function_name.split("::")[-1]
    if short_name and short_name[0].islower():
        return "internal_helper_method"

    if short_name.startswith("~") or re.match(r"^[A-Z][A-Za-z0-9_]*$", short_name):
        return "constructor_or_destructor_like"

    if re.match(r"^(get|set)[A-Z_]", short_name):
        return "accessor_or_mutator"

    if short_name in {"Initialize", "Deinitialize", "Information", "Activated", "Deactivated"}:
        return "plugin_lifecycle_wrapper"

    if short_name in {
        "getInstance",
        "Register",
        "Unregister",
        "Dispatch",
        "dispatchEvent",
    }:
        return "singleton_or_registration_helper"

    if re.match(r"^(on|handle|create|release|fetch|check|process|populate|try|notify|add|remove|read|install|generate)", short_name):
        return "internal_helper_method"

    if short_name in {
        "mkdirRecursive",
        "createAppStorageInfoByAppID",
        "retrieveAppStorageInfoByAppID",
        "removeAppStorageInfoByAppID",
        "hasEnoughStorageFreeSpace",
        "deleteDirectoryEntries",
        "downloadFile",
        "progressCb",
        "write_data",
        "downloaderRoutine",
        "notifyDownloadStatus",
        "pickDownloadJob",
    }:
        return "internal_helper_method"

    if short_name.startswith("On"):
        return "event_callback"

    if short_name.startswith("on"):
        return "event_callback"

    return "missing_dispatch_mapping"


def build_dispatch_coverage_report(targets: List["FunctionTarget"], mapping_file: Path) -> Dict[str, object]:
    by_name: Dict[str, FunctionTarget] = {}
    for target in targets:
        if target.function_name not in by_name:
            by_name[target.function_name] = target

    unique_names = sorted(by_name.keys())
    mapped = []
    unmapped = []

    for name in unique_names:
        t = by_name[name]
        entry = {
            "function_name": name,
            "source_file": t.source_file,
            "line_number": t.line_number,
            "folder_tag": t.folder_tag,
        }
        if is_dispatch_supported(name):
            mapped.append(entry)
        else:
            entry["reason"] = unmapped_reason(name)
            unmapped.append(entry)

    reason_counts: Dict[str, int] = {}
    for entry in unmapped:
        reason = entry["reason"]
        reason_counts[reason] = reason_counts.get(reason, 0) + 1

    return {
        "mapping_file": str(mapping_file),
        "dispatch_supported_count": len(DISPATCH_SUPPORTED_TARGETS),
        "discovered_unique_count": len(unique_names),
        "mapped_unique_count": len(mapped),
        "unmapped_unique_count": len(unmapped),
        "unmapped_reason_counts": reason_counts,
        "mapped_targets": mapped,
        "unmapped_targets": unmapped,
    }


@dataclass
class FunctionTarget:
    function_name: str
    signature: str
    arguments: List[Dict[str, str]]
    source_file: str
    line_number: int
    folder_tag: str
    classification_tags: List[str]
    json_touching: bool
    stable_target_key: str


def parse_args(args_str: str) -> List[Dict[str, str]]:
    args = []
    clean = args_str.strip()
    if not clean or clean == "void":
        return args
    for item in [a.strip() for a in clean.split(",") if a.strip()]:
        part = re.sub(r"\s*=\s*.*$", "", item)
        bits = part.split()
        if len(bits) == 1:
            arg_type = bits[0]
            arg_name = f"arg{len(args)}"
        else:
            arg_name = bits[-1].replace("&", "").replace("*", "")
            arg_type = " ".join(bits[:-1])
        args.append({"name": arg_name, "type": arg_type.strip()})
    return args


def classify(function_name: str, signature: str, source_text: str) -> Tuple[List[str], bool]:
    lower_name = function_name.lower()
    lower_sig = signature.lower()
    lower_src = source_text.lower()
    tags = []
    for word, tag in [
        ("event", "event"),
        ("schedule", "scheduler"),
        ("timer", "scheduler"),
        ("validate", "validation"),
        ("parse", "parsing"),
        ("scan", "monitor"),
        ("monitor", "monitor"),
        ("helper", "helper"),
        ("malloc", "memory"),
        ("memcpy", "memory"),
        ("free", "memory"),
        ("api", "api"),
    ]:
        if word in lower_name or word in lower_sig:
            tags.append(tag)
    if "static" in lower_sig:
        tags.append("static")
    if "::" in function_name:
        tags.append("member")
    if "helper" not in tags and "utils" in lower_src:
        tags.append("helper")
    if not tags:
        tags.append("callable")

    json_touching = any(
        token in (lower_name + " " + lower_sig + " " + lower_src)
        for token in ["json", "cjson", "jsonobject", "payload", "parse", "serialize"]
    )
    return sorted(set(tags)), json_touching


def stable_key(rel_path: str, function_name: str, line: int) -> str:
    src = f"{rel_path}:{function_name}:{line}".encode("utf-8")
    return hashlib.sha1(src).hexdigest()[:16]


def parse_scope(scope_arg: str) -> List[str]:
    scope = [s.strip() for s in scope_arg.split(",") if s.strip()]
    return sorted(set(scope))


def scan_scope(repo_root: Path, output_manifest: Path, include_folders: List[str]) -> None:
    roots = [repo_root / folder for folder in include_folders]
    existing_roots = [p for p in roots if p.exists() and p.is_dir()]
    if not existing_roots:
        raise SystemExit(f"No scope folders found: {', '.join(include_folders)}")

    targets: List[FunctionTarget] = []
    matched_lines = set()
    gaps = []

    scan_files = []
    for root in existing_roots:
        scan_files.extend([p for p in root.rglob("*") if p.suffix in {".cpp", ".h", ".cc", ".hpp"}])
    scan_files = sorted(scan_files)

    for path in scan_files:
        rel_path = str(path.relative_to(repo_root))
        text = path.read_text(encoding="utf-8", errors="ignore")
        lines = text.splitlines()
        folder_tag = rel_path.split("/")[0]

        for idx, line in enumerate(lines, start=1):
            m = FUNCTION_RE.match(line)
            if not m:
                continue
            if not is_probable_function(line, m.group("ret"), m.group("name"), m.group("args")):
                continue
            fname = m.group("name")
            signature = line.strip()
            args = parse_args(m.group("args"))
            tags, json_touch = classify(fname, signature, text)
            key = stable_key(rel_path, fname, idx)
            targets.append(
                FunctionTarget(
                    function_name=fname,
                    signature=signature,
                    arguments=args,
                    source_file=rel_path,
                    line_number=idx,
                    folder_tag=folder_tag,
                    classification_tags=tags,
                    json_touching=json_touch,
                    stable_target_key=key,
                )
            )
            matched_lines.add((rel_path, idx))

        for idx, line in enumerate(lines, start=1):
            if "(" not in line or ")" not in line or "{" not in line:
                continue
            if (rel_path, idx) in matched_lines:
                continue
            if CALLABLE_LIKE_RE.search(line):
                gaps.append(
                    {
                        "source_file": rel_path,
                        "line_number": idx,
                        "reason": "callable-like line unmatched by primary parser",
                        "line": line.strip()[:180],
                    }
                )

    existing_names = {t.function_name for t in targets}

    # Add explicit synthetic targets for interface-map generated refcount methods
    # only when AppManager is in scope.
    if "AppManager" in include_folders:
        impl_h = repo_root / "AppManager" / "AppManagerImplementation.h"
        impl_rel = str(impl_h.relative_to(repo_root))
        impl_line = 1
        if impl_h.exists():
            for idx, line in enumerate(impl_h.read_text(encoding="utf-8", errors="ignore").splitlines(), start=1):
                if "BEGIN_INTERFACE_MAP(AppManagerImplementation)" in line:
                    impl_line = idx
                    break

        for synth in ["AppManagerImplementation::AddRef", "AppManagerImplementation::Release"]:
            if synth in existing_names:
                continue
            key = stable_key(impl_rel, synth, impl_line)
            targets.append(
                FunctionTarget(
                    function_name=synth,
                    signature=f"/* synthetic target for {synth} */",
                    arguments=[],
                    source_file=impl_rel,
                    line_number=impl_line,
                    folder_tag="AppManager",
                    classification_tags=["member", "synthetic"],
                    json_touching=False,
                    stable_target_key=key,
                )
            )

    targets = sorted(targets, key=lambda t: (t.source_file, t.line_number, t.function_name))
    payload = {
        "scope": {
            "include": include_folders,
            "exclude": ["Tests"],
        },
        "target_count": len(targets),
        "targets": [asdict(t) for t in targets],
        "coverage_gaps": gaps,
        "dispatch_summary": {
            "mapping_file": str(DEFAULT_DISPATCH_MAPPING_FILE),
            "dispatch_supported_count": len(DISPATCH_SUPPORTED_TARGETS),
        },
    }

    output_manifest.parent.mkdir(parents=True, exist_ok=True)
    output_manifest.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    dispatch_report = build_dispatch_coverage_report(targets, DEFAULT_DISPATCH_MAPPING_FILE)
    dispatch_report_path = output_manifest.parent / "dispatch_coverage_report.json"
    dispatch_report_path.write_text(json.dumps(dispatch_report, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def safe_cpp_identifier(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", value)


def write_harnesses(repo_root: Path, manifest_path: Path, generated_dir: Path, metadata_map: Path) -> None:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    targets = manifest.get("targets", [])
    generated_dir.mkdir(parents=True, exist_ok=True)

    common_header = generated_dir / "fuzz_harness_common.h"
    common_header.write_text(
        """#pragma once

#include <cstdint>
#include <string>

#if defined(FUZZ_FOLDER_APPMANAGER)
#include "AppManager/AppManagerImplementation.h"
#elif defined(FUZZ_FOLDER_APPSTORAGEMANAGER)
#include "AppStorageManager/AppStorageManagerImplementation.h"
#include "AppStorageManager/RequestHandler.h"
#include "AppStorageManager/AppStorageManagerTelemetryReporting.h"
#elif defined(FUZZ_FOLDER_DOWNLOADMANAGER)
#include "DownloadManager/DownloadManagerImplementation.h"
#elif defined(FUZZ_FOLDER_LIFECYCLEMANAGER)
#include "LifecycleManager/LifecycleManagerImplementation.h"
#elif defined(FUZZ_FOLDER_PACKAGEMANAGER)
#include "PackageManager/PackageManagerImplementation.h"
#elif defined(FUZZ_FOLDER_PREINSTALLMANAGER)
#include "PreinstallManager/PreinstallManagerImplementation.h"
#elif defined(FUZZ_FOLDER_RDKWINDOWMANAGER)
#include "RDKWindowManager/RDKWindowManagerImplementation.h"
#elif defined(FUZZ_FOLDER_TELEMETRYMETRICS)
#include "TelemetryMetrics/TelemetryMetricsImplementation.h"
#endif

namespace fuzzauto {

class InputReader {
public:
    InputReader(const uint8_t* data, size_t size)
        : _data(data)
        , _size(size)
        , _offset(0) {
    }

    bool HasBytes(size_t n) const {
        return (_offset + n) <= _size;
    }

    uint8_t NextByte() {
        if (!HasBytes(1)) {
            return 0;
        }
        return _data[_offset++];
    }

    bool NextBool() {
        return 0 != (NextByte() & 0x1);
    }

    int32_t NextInt32() {
        if (!HasBytes(4)) {
            return 0;
        }
        int32_t v = 0;
        v |= static_cast<int32_t>(_data[_offset]);
        v |= static_cast<int32_t>(_data[_offset + 1]) << 8;
        v |= static_cast<int32_t>(_data[_offset + 2]) << 16;
        v |= static_cast<int32_t>(_data[_offset + 3]) << 24;
        _offset += 4;
        return v;
    }

    std::string NextString(size_t maxLen = 64) {
        if (!HasBytes(1)) {
            return std::string();
        }
        size_t len = static_cast<size_t>(NextByte()) % (maxLen + 1);
        if (!HasBytes(len)) {
            len = _size - _offset;
        }
        std::string out;
        out.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            char c = static_cast<char>(NextByte());
            if ('\\0' == c) {
                c = '_';
            }
            out.push_back(c);
        }
        return out;
    }

private:
    const uint8_t* _data;
    size_t _size;
    size_t _offset;
};

inline bool Contains(const std::string& haystack, const char* needle) {
    return haystack.find(needle) != std::string::npos;
}

template <typename T>
inline T* FuzzPointer(uint8_t mode, T* validPtr = nullptr) {
    if (mode == 0) {
        return nullptr;
    }
    if (mode == 1) {
        return reinterpret_cast<T*>(static_cast<uintptr_t>(0x1));
    }
    return validPtr;
}

inline void RunTargetApi(const std::string& target, InputReader& in) {
    uint8_t ptrMode = static_cast<uint8_t>(in.NextByte() % 3);
    std::string appId = in.NextString();
    std::string intent = in.NextString();
    std::string arg = in.NextString();
    std::string out;
    std::string path;
    std::string error;
    bool installed = false;
    int32_t metric = 0;
    uint32_t size = static_cast<uint32_t>(in.NextInt32());
    uint32_t used = 0;
    int32_t uid = in.NextInt32();
    int32_t gid = in.NextInt32();
    uint64_t requestTime = static_cast<uint64_t>(static_cast<uint32_t>(in.NextInt32()));
    uint8_t percent = 0;
    bool success = false;
    bool keyRepeat = false;
    bool visible = false;
    uint32_t quotaKb = 0;
    uint32_t usedKb = 0;
    auto* shellPtr = FuzzPointer<WPEFramework::PluginHost::IShell>(ptrMode, nullptr);

    (void)target;

#if defined(FUZZ_FOLDER_APPMANAGER)
    auto& impl = *new WPEFramework::Plugin::AppManagerImplementation();
    if (Contains(target, "GetLoadedApps") || Contains(target, "getLoadedApps")) {
        WPEFramework::Exchange::IAppManager::ILoadedAppInfoIterator* apps = nullptr;
        (void)impl.GetLoadedApps(apps);
        if (nullptr != apps) {
            apps->Release();
        }
    } else if ((target == "AppManagerImplementation::LaunchApp") || (target == "LaunchApp")) {
        (void)impl.LaunchApp(appId, intent, arg);
    } else if ((target == "AppManagerImplementation::PreloadApp") || (target == "PreloadApp")) {
        (void)impl.PreloadApp(appId, intent, arg, out);
    } else if ((target == "AppManagerImplementation::CloseApp") || (target == "CloseApp")) {
        (void)impl.CloseApp(appId);
    } else if ((target == "AppManagerImplementation::TerminateApp") || (target == "TerminateApp")) {
        (void)impl.TerminateApp(appId);
    } else if ((target == "AppManagerImplementation::KillApp") || (target == "KillApp")) {
        (void)impl.KillApp(appId);
    } else if ((target == "AppManagerImplementation::SendIntent") || (target == "SendIntent")) {
        (void)impl.SendIntent(appId, intent);
    } else if ((target == "AppManagerImplementation::GetInstalledApps") || (target == "GetInstalledApps")) {
        (void)impl.GetInstalledApps(out);
    } else if ((target == "AppManagerImplementation::IsInstalled") || (target == "IsInstalled")) {
        (void)impl.IsInstalled(appId, installed);
    } else if ((target == "AppManagerImplementation::GetAppProperty") || (target == "GetAppProperty")) {
        (void)impl.GetAppProperty(appId, intent, out);
    } else if ((target == "AppManagerImplementation::SetAppProperty") || (target == "SetAppProperty")) {
        (void)impl.SetAppProperty(appId, intent, arg);
    } else if ((target == "AppManagerImplementation::GetAppMetadata") || (target == "GetAppMetadata")) {
        (void)impl.GetAppMetadata(appId, intent, out);
    } else if ((target == "AppManagerImplementation::StartSystemApp") || (target == "StartSystemApp")) {
        (void)impl.StartSystemApp(appId);
    } else if ((target == "AppManagerImplementation::StopSystemApp") || (target == "StopSystemApp")) {
        (void)impl.StopSystemApp(appId);
    } else if ((target == "AppManagerImplementation::ClearAppData") || (target == "ClearAppData")) {
        (void)impl.ClearAppData(appId);
    } else if ((target == "AppManagerImplementation::ClearAllAppData") || (target == "ClearAllAppData")) {
        (void)impl.ClearAllAppData();
    } else if ((target == "AppManagerImplementation::GetMaxRunningApps") || (target == "GetMaxRunningApps")) {
        (void)impl.GetMaxRunningApps(metric);
    } else if ((target == "AppManagerImplementation::GetMaxHibernatedApps") || (target == "GetMaxHibernatedApps")) {
        (void)impl.GetMaxHibernatedApps(metric);
    } else if ((target == "AppManagerImplementation::GetMaxHibernatedFlashUsage") || (target == "GetMaxHibernatedFlashUsage")) {
        (void)impl.GetMaxHibernatedFlashUsage(metric);
    } else if ((target == "AppManagerImplementation::GetMaxInactiveRamUsage") || (target == "GetMaxInactiveRamUsage")) {
        (void)impl.GetMaxInactiveRamUsage(metric);
    } else {
        (void)impl.KillApp(appId);
        (void)impl.TerminateApp(appId);
        (void)impl.CloseApp(appId);
        (void)impl.SendIntent(appId, intent);
        (void)impl.GetInstalledApps(out);
    }
#elif defined(FUZZ_FOLDER_APPSTORAGEMANAGER)
    auto& storageImpl = *new WPEFramework::Plugin::StorageManagerImplementation();
    auto& requestHandler = WPEFramework::Plugin::RequestHandler::getInstance();
    auto& appStorageTelemetry = WPEFramework::Plugin::AppStorageManagerTelemetryReporting::getInstance();
    if ((target == "AppStorageManager::Initialize") || (target == "Initialize")) {
        appStorageTelemetry.initialize(shellPtr);
        requestHandler.setCurrentService(shellPtr);
        (void)storageImpl.Configure(shellPtr);
    } else if ((target == "AppStorageManager::Deinitialize") || (target == "Deinitialize")) {
        appStorageTelemetry.reset();
        requestHandler.releasePersistentStoreRemoteStoreObject();
    } else if ((target == "AppStorageManager::Information") || (target == "Information")) {
        (void)storageImpl.GetStorage(appId, uid, gid, path, size, used);
    } else if ((target == "StorageManagerImplementation::CreateStorage") || (target == "CreateStorage")) {
        (void)storageImpl.CreateStorage(appId, size, path, error);
    } else if ((target == "StorageManagerImplementation::GetStorage") || (target == "GetStorage")) {
        (void)storageImpl.GetStorage(appId, uid, gid, path, size, used);
    } else if ((target == "StorageManagerImplementation::DeleteStorage") || (target == "DeleteStorage")) {
        (void)storageImpl.DeleteStorage(appId, error);
    } else if ((target == "StorageManagerImplementation::Clear") || (target == "Clear")) {
        (void)storageImpl.Clear(appId, error);
    } else if ((target == "StorageManagerImplementation::ClearAll") || (target == "ClearAll")) {
        (void)storageImpl.ClearAll(arg, error);
    } else if ((target == "StorageManagerImplementation::Configure") || (target == "Configure")) {
        (void)storageImpl.Configure(shellPtr);
    } else if (target == "RequestHandler::SetBaseStoragePath") {
        requestHandler.SetBaseStoragePath(path);
    } else if (target == "RequestHandler::setCurrentService") {
        requestHandler.setCurrentService(nullptr);
    } else if (target == "RequestHandler::CreateStorage") {
        (void)requestHandler.CreateStorage(appId, size, path, error);
    } else if (target == "RequestHandler::GetStorage") {
        (void)requestHandler.GetStorage(appId, uid, gid, path, size, used);
    } else if (target == "RequestHandler::DeleteStorage") {
        (void)requestHandler.DeleteStorage(appId, error);
    } else if (target == "RequestHandler::Clear") {
        (void)requestHandler.Clear(appId, error);
    } else if (target == "RequestHandler::ClearAll") {
        (void)requestHandler.ClearAll(arg, error);
    } else if (target == "RequestHandler::populateAppInfoCacheFromStoragePath") {
        (void)requestHandler.populateAppInfoCacheFromStoragePath();
    } else if (target == "RequestHandler::createPersistentStoreRemoteStoreObject") {
        (void)requestHandler.createPersistentStoreRemoteStoreObject();
    } else if (target == "RequestHandler::releasePersistentStoreRemoteStoreObject") {
        requestHandler.releasePersistentStoreRemoteStoreObject();
    } else if (target == "RequestHandler::isValidAppStorageDirectory") {
        (void)requestHandler.isValidAppStorageDirectory(appId);
    } else if (target == "RequestHandler::appQuotaSizeProperty") {
        uint32_t quotaSize = size;
        uint32_t* quotaPtr = FuzzPointer<uint32_t>(ptrMode, &quotaSize);
        (void)requestHandler.appQuotaSizeProperty(WPEFramework::Plugin::RequestHandler::StorageActionType::GET, appId, quotaPtr);
    } else if (target == "AppStorageManagerTelemetryReporting::initialize") {
        appStorageTelemetry.initialize(shellPtr);
    } else if (target == "AppStorageManagerTelemetryReporting::reset") {
        appStorageTelemetry.reset();
    } else if (target == "AppStorageManagerTelemetryReporting::recordGetStorageTelemetry") {
        appStorageTelemetry.recordGetStorageTelemetry(appId, requestTime);
    } else {
        (void)storageImpl.CreateStorage(appId, size, path, error);
        (void)storageImpl.GetStorage(appId, uid, gid, path, size, used);
        (void)storageImpl.DeleteStorage(appId, error);
    }
#elif defined(FUZZ_FOLDER_DOWNLOADMANAGER)
    auto& downloadImpl = *new WPEFramework::Plugin::DownloadManagerImplementation();
    auto& downloadTelemetry = WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance();
    if ((target == "DownloadManagerImplementation::Initialize") || (target == "Initialize")) {
        (void)downloadImpl.Initialize(shellPtr);
    } else if ((target == "DownloadManagerImplementation::Deinitialize") || (target == "Deinitialize")) {
        (void)downloadImpl.Deinitialize(shellPtr);
    } else if ((target == "DownloadManagerImplementation::Pause") || (target == "Pause")) {
        (void)downloadImpl.Pause(appId);
    } else if ((target == "DownloadManagerImplementation::Resume") || (target == "Resume")) {
        (void)downloadImpl.Resume(appId);
    } else if ((target == "DownloadManagerImplementation::Cancel") || (target == "Cancel")) {
        (void)downloadImpl.Cancel(appId);
    } else if ((target == "DownloadManagerImplementation::Delete") || (target == "Delete")) {
        (void)downloadImpl.Delete(path);
    } else if ((target == "DownloadManagerImplementation::Progress") || (target == "Progress")) {
        (void)downloadImpl.Progress(appId, percent);
    } else if ((target == "DownloadManagerImplementation::RateLimit") || (target == "RateLimit")) {
        (void)downloadImpl.RateLimit(appId, static_cast<uint32_t>(size));
    } else if (target == "DownloadManagerTelemetryReporting::initialize") {
        downloadTelemetry.initialize(shellPtr);
    } else if (target == "DownloadManagerTelemetryReporting::reset") {
        downloadTelemetry.reset();
    } else if (target == "DownloadManagerTelemetryReporting::recordDownloadTimeTelemetry") {
        downloadTelemetry.recordDownloadTimeTelemetry(appId, static_cast<int64_t>(size));
    } else if (target == "DownloadManagerTelemetryReporting::recordDownloadErrorTelemetry") {
        downloadTelemetry.recordDownloadErrorTelemetry(appId, static_cast<int>(uid));
    } else {
        (void)downloadImpl.Initialize(nullptr);
        (void)downloadImpl.Pause(appId);
        (void)downloadImpl.Resume(appId);
        (void)downloadImpl.Cancel(appId);
    }
#elif defined(FUZZ_FOLDER_LIFECYCLEMANAGER)
    auto& lifecycleImpl = *new WPEFramework::Plugin::LifecycleManagerImplementation();
    if ((target == "LifecycleManagerImplementation::GetLoadedApps") || (target == "GetLoadedApps")) {
        (void)lifecycleImpl.GetLoadedApps(in.NextBool(), out);
    } else if ((target == "LifecycleManagerImplementation::IsAppLoaded") || (target == "IsAppLoaded")) {
        (void)lifecycleImpl.IsAppLoaded(appId, success);
    } else if ((target == "LifecycleManagerImplementation::UnloadApp") || (target == "UnloadApp")) {
        (void)lifecycleImpl.UnloadApp(appId, error, success);
    } else if ((target == "LifecycleManagerImplementation::KillApp") || (target == "KillApp")) {
        (void)lifecycleImpl.KillApp(appId, error, success);
    } else if ((target == "LifecycleManagerImplementation::SendIntentToActiveApp") || (target == "SendIntentToActiveApp")) {
        (void)lifecycleImpl.SendIntentToActiveApp(appId, intent, error, success);
    } else if ((target == "LifecycleManagerImplementation::AppReady") || (target == "AppReady")) {
        (void)lifecycleImpl.AppReady(appId);
    } else if ((target == "LifecycleManagerImplementation::StateChangeComplete") || (target == "StateChangeComplete")) {
        (void)lifecycleImpl.StateChangeComplete(appId, static_cast<uint32_t>(size), in.NextBool());
    } else if ((target == "LifecycleManagerImplementation::CloseApp") || (target == "CloseApp")) {
        (void)lifecycleImpl.CloseApp(appId, static_cast<WPEFramework::Exchange::ILifecycleManagerState::AppCloseReason>(0));
    } else {
        (void)lifecycleImpl.GetLoadedApps(in.NextBool(), out);
        (void)lifecycleImpl.IsAppLoaded(appId, success);
        (void)lifecycleImpl.AppReady(appId);
    }
#elif defined(FUZZ_FOLDER_PACKAGEMANAGER)
    auto& packageImpl = *new WPEFramework::Plugin::PackageManagerImplementation();
    if ((target == "PackageManagerImplementation::Initialize") || (target == "Initialize")) {
        (void)packageImpl.Initialize(shellPtr);
    } else if ((target == "PackageManagerImplementation::Deinitialize") || (target == "Deinitialize")) {
        (void)packageImpl.Deinitialize(shellPtr);
    } else if ((target == "PackageManagerImplementation::Pause") || (target == "Pause")) {
        (void)packageImpl.Pause(appId);
    } else if ((target == "PackageManagerImplementation::Resume") || (target == "Resume")) {
        (void)packageImpl.Resume(appId);
    } else if ((target == "PackageManagerImplementation::Cancel") || (target == "Cancel")) {
        (void)packageImpl.Cancel(appId);
    } else if ((target == "PackageManagerImplementation::Delete") || (target == "Delete")) {
        (void)packageImpl.Delete(path);
    } else if ((target == "PackageManagerImplementation::GetStorageInformation") || (target == "GetStorageInformation")) {
        (void)packageImpl.GetStorageInformation(quotaKb, usedKb);
    } else if ((target == "PackageManagerImplementation::RateLimit") || (target == "RateLimit")) {
        (void)packageImpl.RateLimit(appId, static_cast<uint64_t>(size));
    } else if ((target == "PackageManagerImplementation::Uninstall") || (target == "Uninstall")) {
        (void)packageImpl.Uninstall(appId, error);
    } else {
        (void)packageImpl.Initialize(nullptr);
        (void)packageImpl.GetStorageInformation(quotaKb, usedKb);
        (void)packageImpl.RateLimit(appId, static_cast<uint64_t>(size));
    }
#elif defined(FUZZ_FOLDER_PREINSTALLMANAGER)
    auto& preinstallImpl = *new WPEFramework::Plugin::PreinstallManagerImplementation();
    if ((target == "PreinstallManagerImplementation::StartPreinstall") || (target == "StartPreinstall")) {
        (void)preinstallImpl.StartPreinstall(in.NextBool());
    } else if ((target == "PreinstallManagerImplementation::GetPreinstallState") || (target == "GetPreinstallState")) {
        WPEFramework::Exchange::IPreinstallManager::State preState = static_cast<WPEFramework::Exchange::IPreinstallManager::State>(0);
        (void)preinstallImpl.GetPreinstallState(preState);
    } else if ((target == "PreinstallManagerImplementation::Configure") || (target == "Configure")) {
        (void)preinstallImpl.Configure(shellPtr);
    } else {
        WPEFramework::Exchange::IPreinstallManager::State preState = static_cast<WPEFramework::Exchange::IPreinstallManager::State>(0);
        (void)preinstallImpl.GetPreinstallState(preState);
        (void)preinstallImpl.StartPreinstall(in.NextBool());
    }
#elif defined(FUZZ_FOLDER_RDKWINDOWMANAGER)
    auto& rdkWindowImpl = *new WPEFramework::Plugin::RDKWindowManagerImplementation();
    if ((target == "RDKWindowManagerImplementation::Initialize") || (target == "Initialize")) {
        (void)rdkWindowImpl.Initialize(shellPtr);
    } else if ((target == "RDKWindowManagerImplementation::Deinitialize") || (target == "Deinitialize")) {
        (void)rdkWindowImpl.Deinitialize(shellPtr);
    } else if ((target == "RDKWindowManagerImplementation::GetApps") || (target == "GetApps")) {
        (void)rdkWindowImpl.GetApps(out);
    } else if ((target == "RDKWindowManagerImplementation::SetFocus") || (target == "SetFocus")) {
        (void)rdkWindowImpl.SetFocus(appId);
    } else if ((target == "RDKWindowManagerImplementation::SetVisible") || (target == "SetVisible")) {
        (void)rdkWindowImpl.SetVisible(appId, in.NextBool());
    } else if ((target == "RDKWindowManagerImplementation::GetVisibility") || (target == "GetVisibility")) {
        (void)rdkWindowImpl.GetVisibility(appId, visible);
    } else if ((target == "RDKWindowManagerImplementation::EnableKeyRepeats") || (target == "EnableKeyRepeats")) {
        (void)rdkWindowImpl.EnableKeyRepeats(in.NextBool());
    } else if ((target == "RDKWindowManagerImplementation::GetKeyRepeatsEnabled") || (target == "GetKeyRepeatsEnabled")) {
        (void)rdkWindowImpl.GetKeyRepeatsEnabled(keyRepeat);
    } else if ((target == "RDKWindowManagerImplementation::ResetInactivityTime") || (target == "ResetInactivityTime")) {
        (void)rdkWindowImpl.ResetInactivityTime();
    } else if ((target == "RDKWindowManagerImplementation::CreateDisplay") || (target == "CreateDisplay")) {
        (void)rdkWindowImpl.CreateDisplay(appId, intent, static_cast<uint32_t>(size), static_cast<uint32_t>(size), in.NextBool(), static_cast<uint32_t>(size), static_cast<uint32_t>(size), static_cast<uint32_t>(uid), static_cast<uint32_t>(gid), in.NextBool(), in.NextBool());
    } else {
        (void)rdkWindowImpl.GetApps(out);
        (void)rdkWindowImpl.SetFocus(appId);
        (void)rdkWindowImpl.SetVisible(appId, in.NextBool());
        (void)rdkWindowImpl.ResetInactivityTime();
    }
#elif defined(FUZZ_FOLDER_TELEMETRYMETRICS)
    auto& telemetryImpl = *new WPEFramework::Plugin::TelemetryMetricsImplementation();
    if ((target == "TelemetryMetricsImplementation::Record") || (target == "Record")) {
        (void)telemetryImpl.Record(appId, intent, arg);
    } else if ((target == "TelemetryMetricsImplementation::Publish") || (target == "Publish")) {
        (void)telemetryImpl.Publish(appId, intent);
    } else {
        (void)telemetryImpl.Record(appId, intent, arg);
        (void)telemetryImpl.Publish(appId, intent);
    }
#else
    (void)appId;
#endif
}

inline int FuzzHarnessRunForTarget(const char* targetName, const uint8_t* data, size_t size) {
    if ((nullptr == data) || (0 == size) || (nullptr == targetName)) {
        return 0;
    }

    InputReader in(data, size);
    RunTargetApi(std::string(targetName), in);
    return 0;
}

} // namespace fuzzauto
""",
        encoding="utf-8",
    )

    metadata = {}
    if metadata_map.exists():
        try:
            metadata = json.loads(metadata_map.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            metadata = {}

    scoped_folders = set(manifest.get("scope", {}).get("include", DEFAULT_SCOPE_FOLDERS))
    target_keys = {t["stable_target_key"] for t in targets}

    # Remove stale harnesses only for folders included in this run's scope.
    stale_keys = []
    for key, item in metadata.items():
        source_file = item.get("source_file", "")
        folder = source_file.split("/")[0] if source_file else ""
        if (folder in scoped_folders) and (key not in target_keys):
            stale_keys.append(key)

    for key in stale_keys:
        harness_rel = metadata.get(key, {}).get("harness")
        if harness_rel:
            harness_path = repo_root / harness_rel
            if harness_path.exists():
                harness_path.unlink()
        metadata.pop(key, None)

    for target in targets:
        key = target["stable_target_key"]
        name = target["function_name"]
        src = target["source_file"]
        folder = src.split("/")[0] if src else ""
        folder_macro = f"FUZZ_FOLDER_{safe_cpp_identifier(folder).upper()}" if folder else ""
        harness_path = generated_dir / f"fuzz_{safe_cpp_identifier(key)}.cpp"
        metadata[key] = {
            "function_name": name,
            "source_file": src,
            "line_number": target["line_number"],
            "classification_tags": target["classification_tags"],
            "json_touching": target["json_touching"],
            "harness": str(harness_path.relative_to(repo_root)),
        }

        harness = f"""// Auto-generated libFuzzer harness for {name} ({src}:{target['line_number']})

#include <cstddef>
#include <cstdint>

#define {folder_macro}
#include "fuzz_harness_common.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {{
    return fuzzauto::FuzzHarnessRunForTarget("{name}", data, size);
}}
"""
        harness_path.write_text(harness, encoding="utf-8")

    metadata_map.parent.mkdir(parents=True, exist_ok=True)
    metadata_map.write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def generate_cmake(
    repo_root: Path,
    manifest_path: Path,
    generated_dir: Path,
    stubs_dir: Path,
    out_cmake: Path,
    dependencies_root: Path,
) -> None:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    targets = manifest.get("targets", [])
    include_scope = manifest.get("scope", {}).get("include", DEFAULT_SCOPE_FOLDERS)
    dispatch_folders = {
        t.get("source_file", "").split("/")[0]
        for t in targets
        if t.get("source_file")
    }
    app_sources = []
    folder_sources = {}
    for folder in include_scope:
        if folder not in dispatch_folders:
            continue
        folder_path = repo_root / folder
        if not folder_path.exists() or not folder_path.is_dir():
            continue
        excluded_for_fuzz = {"Module.cpp", f"{folder}.cpp"}
        folder_cpp_sources = []
        for p in folder_path.glob("*.cpp"):
            if p.name in excluded_for_fuzz:
                continue
            rel_path = str(p.relative_to(repo_root))
            app_sources.append(rel_path)
            folder_cpp_sources.append(rel_path)
        if folder_cpp_sources:
            folder_sources[folder] = sorted(set(folder_cpp_sources))
    app_sources = sorted(set(app_sources))

    harness_specs = []
    for t in targets:
        source_file = t.get("source_file", "")
        owning_folder = source_file.split("/")[0] if source_file else ""
        harness_specs.append(
            {
                "key": safe_cpp_identifier(t["stable_target_key"]),
                "file": f"generated/fuzz_{safe_cpp_identifier(t['stable_target_key'])}.cpp",
                "folder": owning_folder,
            }
        )

    stubs_sources = sorted(str(p.relative_to(repo_root / "fuzz" / "stats" / "auto")) for p in stubs_dir.glob("*.c"))
    global_stubs = [s for s in stubs_sources if "fuzz_global_stubs" in s]
    profile_stubs = [s for s in stubs_sources if "fuzz_global_stubs" not in s]

    lines = [
        "cmake_minimum_required(VERSION 3.16)",
        "project(AppManagerAutoFuzz LANGUAGES C CXX)",
        "set(CMAKE_CXX_STANDARD 17)",
        "set(CMAKE_CXX_STANDARD_REQUIRED ON)",
        "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)",
        "",
        "# Add stubs directory early (BEFORE other includes) to find fallback headers only",
        "include_directories(BEFORE ${CMAKE_SOURCE_DIR}/stubs/include)",
        "",
        "if(NOT CMAKE_CXX_COMPILER_ID MATCHES \"Clang\")",
        "  message(WARNING \"Clang is recommended for libFuzzer builds\")",
        "endif()",
        "",
        "# Workaround for macOS ASAN header issue: ensure stdint types are globally visible",
        "add_compile_options(-include stdint.h)",
        "",
        "# Compiler flags for fuzzing - libFuzzer + sanitizer set",
        "add_compile_options(-fPIC)",
        "add_compile_options(-fno-omit-frame-pointer -g -O1)",
        "if(APPLE)",
        "  # AppleClang does not support -fsanitize=leak on this target.",
        "  set(_FUZZ_DEFAULT_SANITIZERS \"fuzzer,address,undefined\")",
        "else()",
        "  set(_FUZZ_DEFAULT_SANITIZERS \"fuzzer,address,undefined,leak\")",
        "endif()",
        "set(FUZZ_SANITIZERS \"${_FUZZ_DEFAULT_SANITIZERS}\" CACHE STRING \"Sanitizers to enable for fuzz targets\")",
        "add_compile_options(-fsanitize=${FUZZ_SANITIZERS})",
        "add_link_options(-fsanitize=${FUZZ_SANITIZERS})",
        "add_compile_options(-fno-sanitize-recover=all)",
        "",
        "set(FUZZ_DEPENDENCIES_ROOT \"{}\")".format(dependencies_root),
        "option(FUZZ_INCLUDE_APP_SOURCES \"Compile AppManager sources into fuzz targets\" ON)",
        "",
        "# Include paths: order matters on macOS - project root paths first",
        "include_directories(${CMAKE_SOURCE_DIR}/../../..)",
        "include_directories(${CMAKE_SOURCE_DIR}/../../../helpers)",
        "include_directories(${CMAKE_SOURCE_DIR}/stubs)",
        "",
        "# Add dependencies root for API headers",
        "include_directories(${FUZZ_DEPENDENCIES_ROOT}/entservices-apis/apis)",
        "include_directories(${FUZZ_DEPENDENCIES_ROOT})",
        "",
        "# Add explicit include directories for common dependency patterns",
        "file(GLOB DEP_API_DIRS LIST_DIRECTORIES true ${FUZZ_DEPENDENCIES_ROOT}/entservices-apis/apis/*/)",
        "foreach(api_dir ${DEP_API_DIRS})",
        "  if(IS_DIRECTORY ${api_dir})",
        "    include_directories(${api_dir})",
        "  endif()",
        "endforeach()",
        "",
        "# Discover jsoncpp/messaging include roots from dependencies when present.",
        "file(GLOB_RECURSE FUZZ_JSONCPP_HEADERS ${FUZZ_DEPENDENCIES_ROOT}/*/json/json.h)",
        "foreach(json_header ${FUZZ_JSONCPP_HEADERS})",
        "  get_filename_component(_json_subdir ${json_header} DIRECTORY)",
        "  get_filename_component(_json_root ${_json_subdir} DIRECTORY)",
        "  include_directories(${_json_root})",
        "endforeach()",
        "file(GLOB_RECURSE FUZZ_MESSAGING_HEADERS ${FUZZ_DEPENDENCIES_ROOT}/*/messaging/messaging.h)",
        "foreach(messaging_header ${FUZZ_MESSAGING_HEADERS})",
        "  get_filename_component(_messaging_subdir ${messaging_header} DIRECTORY)",
        "  get_filename_component(_messaging_root ${_messaging_subdir} DIRECTORY)",
        "  include_directories(${_messaging_root})",
        "endforeach()",
        "",
        "set(FUZZ_OPTIONAL_LIBS)",
        "find_library(FUZZ_JSONCPP_LIB NAMES jsoncpp PATHS ${FUZZ_DEPENDENCIES_ROOT} PATH_SUFFIXES lib lib64 build/lib NO_DEFAULT_PATH)",
        "if(FUZZ_JSONCPP_LIB)",
        "  list(APPEND FUZZ_OPTIONAL_LIBS ${FUZZ_JSONCPP_LIB})",
        "endif()",
        "find_library(FUZZ_MESSAGING_LIB NAMES messaging WPEFrameworkMessaging PATHS ${FUZZ_DEPENDENCIES_ROOT} PATH_SUFFIXES lib lib64 build/lib NO_DEFAULT_PATH)",
        "if(FUZZ_MESSAGING_LIB)",
        "  list(APPEND FUZZ_OPTIONAL_LIBS ${FUZZ_MESSAGING_LIB})",
        "endif()",
        "",
        "file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)",
        "",
    ]

    for folder in include_scope:
        lines.append(f"include_directories(${{CMAKE_SOURCE_DIR}}/../../../{folder})")

    if app_sources:
        lines.append("set(APP_MANAGER_SOURCES")
        for src in app_sources:
            lines.append(f"  ${{CMAKE_SOURCE_DIR}}/../../../{src}")
        lines.append(")")
    else:
        lines.append("set(APP_MANAGER_SOURCES)")

    lines.append("set(STUB_SOURCES")
    for src in profile_stubs:
        lines.append(f"  ${{CMAKE_SOURCE_DIR}}/{src}")
    lines.append(")")
    lines.append("add_library(appmanager_fuzz_stub_support OBJECT ${STUB_SOURCES})")
    lines.append("set_target_properties(appmanager_fuzz_stub_support PROPERTIES POSITION_INDEPENDENT_CODE ON)")
    lines.append("if(FUZZ_OPTIONAL_LIBS)")
    lines.append("  target_link_libraries(appmanager_fuzz_stub_support PRIVATE ${FUZZ_OPTIONAL_LIBS})")
    lines.append("endif()")
    lines.append("")
    
    # Create static library for global stubs (linked once into each executable, not into libraries)
    if global_stubs:
        lines.append("set(GLOBAL_STUB_SOURCES")
        for src in global_stubs:
            lines.append(f"  ${{CMAKE_SOURCE_DIR}}/{src}")
        lines.append(")")
        lines.append("add_library(appmanager_fuzz_global_stubs STATIC ${GLOBAL_STUB_SOURCES})")
        lines.append("set_target_properties(appmanager_fuzz_global_stubs PROPERTIES POSITION_INDEPENDENT_CODE ON)")
        lines.append("")

    for folder, sources in sorted(folder_sources.items()):
        folder_id = safe_cpp_identifier(folder).lower()
        lines.append(f"set(FUZZ_{folder_id.upper()}_SOURCES")
        for src in sources:
            lines.append(f"  ${{CMAKE_SOURCE_DIR}}/../../../{src}")
        lines.append(")")
        lines.append(f"if(FUZZ_INCLUDE_APP_SOURCES)")
        lines.append(f"  add_library(appmanager_fuzz_{folder_id} SHARED ${{FUZZ_{folder_id.upper()}_SOURCES}})")
        lines.append(f"  set_target_properties(appmanager_fuzz_{folder_id} PROPERTIES")
        lines.append("    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin")
        lines.append("    BUILD_RPATH ${CMAKE_BINARY_DIR}/bin")
        lines.append("  )")
        lines.append(f"  target_sources(appmanager_fuzz_{folder_id} PRIVATE $<TARGET_OBJECTS:appmanager_fuzz_stub_support>)")
        lines.append("  if(FUZZ_OPTIONAL_LIBS)")
        lines.append(f"    target_link_libraries(appmanager_fuzz_{folder_id} PRIVATE ${{FUZZ_OPTIONAL_LIBS}})")
        lines.append("  endif()")
        lines.append("endif()")
        lines.append("")

    for harness in harness_specs:
        h = harness["file"]
        key = harness["key"]
        folder = harness["folder"]
        folder_id = safe_cpp_identifier(folder).lower() if folder else ""
        lines.extend(
            [
                f"add_executable(fuzz_{key}",
                f"  ${{CMAKE_SOURCE_DIR}}/{h}",
                "  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>",
                ")",
                "if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_{})".format(folder_id) if folder_id else "if(FUZZ_INCLUDE_APP_SOURCES)",
                f"  target_link_libraries(fuzz_{key} PRIVATE appmanager_fuzz_{folder_id})" if folder_id else "",
                "endif()",
                "if(FUZZ_OPTIONAL_LIBS)",
                f"  target_link_libraries(fuzz_{key} PRIVATE ${{FUZZ_OPTIONAL_LIBS}})",
                "endif()",
                "if(TARGET appmanager_fuzz_global_stubs)",
                f"  target_link_libraries(fuzz_{key} PRIVATE appmanager_fuzz_global_stubs)",
                "endif()",
                f"set_target_properties(fuzz_{key} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${{CMAKE_BINARY_DIR}}/bin)",
                "",
            ]
        )

    lines = [line for line in lines if line != ""] + [""]

    out_cmake.parent.mkdir(parents=True, exist_ok=True)
    out_cmake.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Auto fuzz discovery and generation")
    parser.add_argument("command", choices=["discover", "generate-harnesses", "generate-cmake"])
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--manifest", default="fuzz/stats/results/discovery_manifest.json")
    parser.add_argument("--generated-dir", default="fuzz/stats/auto/generated")
    parser.add_argument("--metadata-map", default="fuzz/stats/results/target_metadata_map.json")
    parser.add_argument("--stubs-dir", default="fuzz/stats/auto/stubs")
    parser.add_argument("--cmake-out", default="fuzz/stats/auto/CMakeLists.txt")
    parser.add_argument("--dependencies-root", default="/Users/mthiru270@apac.comcast.com/fuzz7/dependencies")
    parser.add_argument("--scope", default=",".join(DEFAULT_SCOPE_FOLDERS))
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    manifest = (repo_root / args.manifest).resolve()
    generated_dir = (repo_root / args.generated_dir).resolve()
    metadata_map = (repo_root / args.metadata_map).resolve()
    stubs_dir = (repo_root / args.stubs_dir).resolve()
    cmake_out = (repo_root / args.cmake_out).resolve()
    dependencies_root = Path(args.dependencies_root).resolve()
    include_scope = parse_scope(args.scope)

    if args.command == "discover":
        scan_scope(repo_root, manifest, include_scope)
    elif args.command == "generate-harnesses":
        write_harnesses(repo_root, manifest, generated_dir, metadata_map)
    elif args.command == "generate-cmake":
        generate_cmake(repo_root, manifest, generated_dir, stubs_dir, cmake_out, dependencies_root)


if __name__ == "__main__":
    main()
