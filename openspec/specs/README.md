# ENT Services AppManagers - Capability Specifications

This directory contains formal capability specifications for the entservices-appmanagers project, organized according to the OpenSpec spec-driven schema.

## 📋 Contents

### Module Specifications

| Module | Purpose | Status | Spec File |
|--------|---------|--------|-----------|
| **AppManager** | Primary API for application lifecycle management | ✅ Formal | [AppManager.spec.md](./AppManager.spec.md) |
| **LifecycleManager** | Application state machine (UNLOADED → ACTIVE → SUSPENDED → TERMINATED) | ✅ Formal | [LifecycleManager.spec.md](./LifecycleManager.spec.md) |
| **RuntimeManager** | OCI container runtime using Dobby | ✅ Formal | [RuntimeManager.spec.md](./RuntimeManager.spec.md) |
| **PackageManager** | Package download, installation, locking, uninstallation | ✅ Formal | [PackageManager.spec.md](./PackageManager.spec.md) |
| **AppStorageManager** | Application-specific persistent storage | ✅ Formal | [AppStorageManager.spec.md](./AppStorageManager.spec.md) |
| **RDKWindowManager** | Display and window management (Wayland) | ✅ Formal | [RDKWindowManager.spec.md](./RDKWindowManager.spec.md) |
| **DownloadManager** | HTTP downloads with queueing, pause/resume, rate limiting | ✅ Formal | [DownloadManager.spec.md](./DownloadManager.spec.md) |
| **PreinstallManager** | Scan and install pre-installed packages | ✅ Formal | [PreinstallManager.spec.md](./PreinstallManager.spec.md) |
| **TelemetryMetrics** | Performance metrics and analytics collection | ✅ Formal | [TelemetryMetrics.spec.md](./TelemetryMetrics.spec.md) |

### Guidelines & References

| Document | Purpose |
|----------|---------|
| [SPEC_TEMPLATE.md](./SPEC_TEMPLATE.md) | Template for creating new module specifications |
| [INFERENCE_GUIDE.md](./INFERENCE_GUIDE.md) | How OpenSpec explore extracts specs from codebase (CMakeLists.txt, headers, code) |
| [OUTPUT_FORMAT_GUIDE.md](./OUTPUT_FORMAT_GUIDE.md) | Professional output format for OpenSpec explore (Dobby-style documentation) |

---

## 🎯 What Are These Specs?

Each module specification documents:

### **Overview**
- Module name and type (Plugin/Framework/Utility)
- Core purpose and responsibilities
- What the module intentionally does NOT do (non-goals)

### **Capabilities**
- **Primary Responsibilities**: Main functions
- **Non-Goals**: Intentionally delegated or out-of-scope

### **Dependencies**
- **External Repositories** (entservices-apis, Dobby, rdk-window-manager, libpackage-sky, eshelpers)
- **System Libraries** (curl, yaml-cpp, jsoncpp, crun, criu)
- **Internal Dependencies** (other modules and helpers)
- **CMake Configuration** (build options and feature flags)

### **Interfaces**
- **Public APIs** (JSON-RPC methods)
- **Configuration Files** (*.conf.in, *.config)
- **Callsigns / JSON-RPC Endpoints** (Thunder/WPEFramework, e.g. `org.rdk.<PluginName>` via `http://127.0.0.1:9998/jsonrpc`)

### **Data Flow**
- Visual representation of how data flows through the module
- How the module integrates with other services

### **Integration Points**
- What the module consumes from
- What the module provides to

### **Build & Installation**
- Compiled artifacts
- Installation paths
- API versions

### **Constraints & Limitations**
- Known constraints
- Runtime dependencies
- Scaling limits

### **Testing**
- Unit test locations
- Integration test locations
- Test dependencies

---

## 🔍 How These Specs Were Created

### Two-Pronged Approach

#### 1. **Manual Formalization** ✓ Completed
- Extracted module purposes from `<MODULE>/<MODULE>.md` documentation
- Analyzed CMakeLists.txt for build configuration and dependencies
- Read header files (`.h`) to identify public API surfaces
- Documented integration patterns from code structure
- Captured data flows from architecture diagrams

#### 2. **Automated Inference** ✓ Guide Provided
See [INFERENCE_GUIDE.md](./INFERENCE_GUIDE.md) for how OpenSpec explore can automatically extract specs from:

- **CMakeLists.txt** → Dependencies, build options, feature flags
- **Header files** (.h) → Public API interfaces, capability methods
- **Module documentation** (.md) → Purpose, architecture, responsibilities
- **Source code** (.cpp) → Integration patterns, module interactions
- **Configuration files** (.conf.in) → Runtime configuration schema

**Key insight**: Specs can be kept in sync with code automatically!

---

## 📊 Dependency Overview

```
entservices-appmanagers (this repository)
├─ AppManager ──────┐
│                    ↓
├─ LifecycleManager ─→ RuntimeManager ──────────┐
│                         │                      ↓
├─ PackageManager ──→ AppStorageManager ← RuntimeManager
│                      ↑
├─ DownloadManager ────┘
│
├─ PreinstallManager ──→ PackageManager
│
├─ RDKWindowManager ←─┐
│                     ├─ RuntimeManager
│                     ├─ LifecycleManager
│
├─ TelemetryMetrics ─→ All modules (optional)
│
└─ AppManagersHelpers (shared utilities)

External Dependencies:
├─ entservices-apis (API interfaces for all)
├─ Dobby (RuntimeManager → OCI container runtime)
├─ rdk-window-manager (RDKWindowManager)
├─ libpackage-sky (PackageManager)
└─ eshelpers (shared utilities)

System Libraries:
├─ WPEFramework (all plugins)
├─ curl (DownloadManager, PackageManager)
├─ yaml-cpp (RuntimeManager, optional)
├─ jsoncpp (all JSON-handling modules)
├─ crun (runtime, Dobby)
└─ criu (runtime, hibernation)
```

---

## 🚀 Using OpenSpec Explore

### Generate Formal Documentation

When OpenSpec explore is run on this codebase, it will generate:

```bash
opsx:explore --output-format dobby-style
```

**Generated Artifacts**:
- `REPOSITORY_SUMMARY.md` - High-level overview
- `REQUIREMENTS.md` - Build & system dependencies
- `MODULES.md` - Module catalog with metrics
- `CMAKE_OPTIONS.md` - Build configuration reference
- `ARCHITECTURE.md` - System architecture and diagrams
- `BUILD_INSTALL.md` - Build and installation instructions
- `API_REFERENCE.md` - Complete API documentation
- `CONFIGURATION.md` - Configuration schema reference
- `TESTING.md` - Testing guide
- `DEPENDENCIES.md` - Dependency analysis matrix

See [OUTPUT_FORMAT_GUIDE.md](./OUTPUT_FORMAT_GUIDE.md) for complete details.

---

## 📝 Spec Maintenance

### When to Update Specs

Update the relevant spec when:

1. **New API method added** to a module
   - Update: `Interfaces.PublicAPIs` section
   - File: `<MODULE>.spec.md`

2. **New build option added** to CMakeLists.txt
   - Update: `Dependencies.CMakeConfiguration` section
   - File: `<MODULE>.spec.md`

3. **New external dependency required**
   - Update: `Dependencies.External Repositories` or `System Libraries`
   - File: `<MODULE>.spec.md`

4. **Module purpose or responsibilities change**
   - Update: `Overview`, `Capabilities` sections
   - File: `<MODULE>.spec.md`

5. **Integration with new module added**
   - Update: `Integration Points`, `Data Flow` sections
   - Files: Both modules' specs

### Validation Checklist

Before considering a spec "complete":

- [ ] All public APIs documented
- [ ] All build options listed
- [ ] All dependencies accounted for
- [ ] Data flow diagram created or updated
- [ ] Integration points documented
- [ ] Non-goals clearly stated
- [ ] Test locations specified
- [ ] Constraints and limitations noted

---

## 🔗 Cross-References

### By External Repository

**entservices-apis**
- Used by: All modules (API interfaces)
- Specs affected: All

**Dobby**
- Used by: RuntimeManager
- Spec: [RuntimeManager.spec.md](./RuntimeManager.spec.md)

**rdk-window-manager**
- Used by: RDKWindowManager, RuntimeManager
- Specs: [RDKWindowManager.spec.md](./RDKWindowManager.spec.md), [RuntimeManager.spec.md](./RuntimeManager.spec.md)

**libpackage-sky**
- Used by: PackageManager
- Spec: [PackageManager.spec.md](./PackageManager.spec.md)

**eshelpers**
- Used by: All modules (optional)
- Specs affected: All

### By System Library

**WPEFramework**
- Used by: All plugins
- Specs affected: All

**curl**
- Used by: DownloadManager, PackageManager
- Specs: [DownloadManager.spec.md](./DownloadManager.spec.md), [PackageManager.spec.md](./PackageManager.spec.md)

**yaml-cpp** (optional)
- Used by: RuntimeManager (if ENABLE_RDKAPPMANAGERS_RUNTIMECONFIG)
- Spec: [RuntimeManager.spec.md](./RuntimeManager.spec.md)

---

## 📚 Related Documentation

- **Repository README**: [../../README.md](../../README.md) - Architecture overview
- **Module Documentation**: `<MODULE>/<MODULE>.md` - Detailed module docs
- **Build System**: [../../CMakeLists.txt](../../CMakeLists.txt) - CMake configuration
- **Tests**: [../../Tests/](../../Tests/) - Unit and integration tests

---

## 🤝 Contributing

To propose changes affecting specifications:

1. **Create an OpenSpec change**: Use `openspec propose` to start a formal change
2. **Reference relevant specs**: Link to affected `.spec.md` files
3. **Update specs in change proposal**: Modify specs as part of change implementation
4. **Keep INFERENCE_GUIDE synchronized**: If changing extraction patterns

---

## 📌 Specification Schema

All specs follow this structure:

```
# <Module Name> Specification

## Overview
- Module name, type, purpose

## Capabilities
- Primary responsibilities
- Non-goals

## Dependencies
- External repositories
- System libraries
- Internal dependencies
- CMake configuration

## Interfaces
- Public APIs
- Configuration
- Notifications

## Data Flow
- ASCII/Mermaid diagrams

## Integration Points
- Consumers
- Providers

## Build & Installation
- Artifacts
- Paths
- Versions

## Constraints & Limitations
- Known limitations
- Design constraints

## Testing
- Test locations
- Dependencies

## Related Documentation
- Links to related docs
```

See [SPEC_TEMPLATE.md](./SPEC_TEMPLATE.md) for the complete template.

---

## 🎓 Learning Resources

- **Module purposes & architecture**: Read individual `<MODULE>.spec.md` files
- **How specs are extracted**: See [INFERENCE_GUIDE.md](./INFERENCE_GUIDE.md)
- **Professional output format**: See [OUTPUT_FORMAT_GUIDE.md](./OUTPUT_FORMAT_GUIDE.md)
- **System-wide architecture**: Read [../../README.md](../../README.md)

---

**Last Updated**: June 6, 2026  
**Schema**: OpenSpec (spec-driven)  
**Status**: Formal specification set ✅
