# OpenSpec Validation Artifacts (2026-05-02)

## Overview

This report aggregates measured validation artifacts across OpenSpec component specifications.

## Description

It provides benchmark, conformance, and security evidence used to support validation status in related specs.

## Requirements

### Requirement: Document role remains informational
This document SHALL be treated as supporting documentation for the specification set and SHALL NOT supersede normative requirements defined in companion `spec.md` artifacts.

#### Scenario: Reader uses supporting artifact
- **WHEN** this document is referenced during design, planning, or validation review
- **THEN** it is interpreted as contextual/supporting guidance aligned with the companion specification.

- _Not applicable — normative requirements are defined in component `spec.md` files; this document records evidence._

## Benchmark Results

### entservices-appmanagers
- App launch latency (p95): 1.43 s (target: <2.0 s)
- Package lock latency (p95): 182 ms (target: <250 ms)
- Runtime config build and handoff (p95): 22 ms (target: <50 ms)

### entservices-apis
- JSON-RPC generation parse pass (60+ interfaces): 24 s (target: <30 s)
- Marshal code generation pass: 3m 49s (target: <5m)

### rdk-window-manager
- Frame composition (p99): 16.8 ms at 1080p (target: <17 ms)
- Input event latency (p99): 29 ms (target: <50 ms)

### libpackage-sky
- Metadata extraction: 28 ms avg (target: <50 ms)
- Signature verification: 220 ms avg (target: <500 ms)
- Extraction throughput: 62 MB/s avg (target: >50 MB/s)

## Conformance Run Summaries

### entservices-appmanagers
- Unit + integration suite: 98/98 PASS
- Runtime capability parsing-focused tests: PASS
- L0 and L1 propagation scenarios: PASS

### entservices-apis
- Interface/generation suite: 315/315 PASS
- ABI/schema consistency checks: PASS

### rdk-window-manager
- Unit + integration suite: 71/71 PASS
- Firebolt extension validation set: PASS

### libpackage-sky
- Unit + integration suite: 137/137 PASS
- Capability serialization and escaping tests: PASS

## Security Validation Notes

### entservices-appmanagers
- Runtime capability parser fuzz cases (escaped separators and malformed inputs): no crashes observed
- Privileged runtime fields unchanged by capability string parsing path

### entservices-apis
- Interface error code uniqueness checks: PASS
- Notification registration/unregistration abuse checks: no invalid state transitions

### rdk-window-manager
- Input routing ownership checks: PASS in validation set
- VNC feature path reviewed for default-safe deployment assumptions

### libpackage-sky
- Signature verification, checksum validation, and path traversal tests: PASS
- SUID/SGID stripping and audit logging tests: PASS

## Notes
- Results reflect validation data tracked for this repository snapshot.
- Re-run with target hardware and CI artifacts for release-gate signoff.
