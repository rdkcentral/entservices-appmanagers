# EntServices-AppManagers - Proposal

## Overview

This proposal describes scope and intent for EntServices-AppManagers specification and planning artifacts.

## Description

It provides proposal-level context on goals, problem framing, and planned outcomes for appmanagers services.

## Requirements

### Requirement: Document role remains informational
This document SHALL be treated as supporting documentation for the specification set and SHALL NOT supersede normative requirements defined in companion `spec.md` artifacts.

#### Scenario: Reader uses supporting artifact
- **WHEN** this document is referenced during design, planning, or validation review
- **THEN** it is interpreted as contextual/supporting guidance aligned with the companion specification.

- _Not applicable — normative requirements are defined in `openspec/specs/entservices-appmanagers/spec.md`; this document captures proposal scope._

## Executive Summary

EntServices-AppManagers implements the concrete service logic for application lifecycle management, package installation, and runtime configuration. It provides the plugin-based services that realize the contracts defined in EntServices-APIs, handling app discovery, installation, state transitions, and persistent storage management.

## Problem Statement

RDK devices need:
- Reliable application lifecycle management (launch, suspend, terminate, uninstall)
- Safe package installation/verification with secure storage
- Pre-installed application management and discovery
- Application state synchronization with window manager
- Telemetry metrics collection for analytics
- Atomic install/uninstall with rollback capability

## Scope

### In-Scope
- AppManager plugin (lifecycle control, app discovery, state notifications)
- LifecycleManager (state machine, transition handlers)
- PackageManager plugin (install, uninstall, verify)
- PreinstallManager (manage pre-installed apps)
- RDKWindowManager integration (surface management)
- TelemetryMetrics (report app lifecycle events)
- AppStorageManager (persistent app storage)
- DownloadManager (async package downloads)

### Out-of-Scope
- Package format specification (libpackage-sky owns this)
- Cryptographic verification (delegated to libpackage-sky)
- Hardware resource management
- Network transport

## Goals

1. **Reliable State Management** - Accurate tracking of all app states
2. **Safe Installation** - Atomic operations with verification and rollback
3. **Performance** - Fast app launch (<2s typical)
4. **Observability** - Complete telemetry for all app operations
5. **Extensibility** - Plugin architecture for custom behaviors

## Non-Goals

- Custom app sandbox implementation (use Linux namespaces/cgroups)
- DRM content protection
- Network optimization
- UI implementation

## Key Components

1. **AppManager** - Main lifecycle and discovery service
2. **LifecycleManager** - State machine and transitions
3. **PackageManager** - Installation and package operations
4. **PreinstallManager** - Pre-installed app management
5. **AppStorageManager** - Application-specific storage
6. **DownloadManager** - Async file downloads
7. **TelemetryMetrics** - Lifecycle event reporting

## Success Metrics

- App launch latency <2 seconds (typical)
- Package installation success >99.5%
- Zero data loss on interrupted install (atomic operations)
- 100% telemetry event capture
- Support 500+ apps in registry
