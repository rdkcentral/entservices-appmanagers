# PreinstallManager Design

## Overview
PreinstallManager performs controlled discovery and install triggering for preloaded package artifacts during startup or on-demand execution.

## Architecture

```text
Preinstall API facade
   |
   v
PreinstallManagerImplementation
   |---- Path scanner
   |---- Candidate parser/validator
   |---- Policy evaluator (force/non-force)
   |---- Package manager installer connector
   |---- Notification dispatcher
```

## Workflow

### Start preinstall
1. Validate invocation and execution state.
2. Enumerate configured directories.
3. Build candidate list with metadata.
4. Evaluate policy against installed state.
5. Invoke package install for eligible candidates.
6. Emit per-candidate and summary notifications.

## Decision Model
- Candidate validity check
- Installed version lookup
- Force/non-force policy branch
- Install attempt and result capture

## Concurrency Model
- Single active preinstall run at a time.
- Additional trigger requests return busy/coalesced status.
- Notification dispatch isolated from scan/install critical path.

## Error Handling
- Filesystem scan errors are categorized and scoped to path/candidate.
- Package install API failures are mapped to canonical taxonomy.
- Workflow summary reports aggregate outcome by candidate.

## Observability
- Logs include directory, package identity, policy decision, and category.
- Metrics include scan duration, candidate count, install count, skip count, failure count.

## Safety
- Candidate parsing validates package metadata before install request.
- Non-force policy prevents unintended downgrade/reinstall.
- Failures on one candidate do not terminate full workflow unless fatal.
