# DownloadManager Design

## Overview
DownloadManager manages file transfer lifecycle with queue orchestration, transport abstraction, and robust status reporting.

## Architecture

```text
Download API facade
   |
   v
DownloadManagerImplementation
   |---- Queue manager
   |---- Scheduler/worker
   |---- HTTP transport adapter
   |---- Progress notifier
   |---- Retry controller
```

## Core Components

### Queue manager
Maintains queued, active, paused, completed, and failed download state entries.

### Scheduler/worker
Selects next work item and executes transfer according to priority/policy.

### HTTP transport adapter
Performs network transfer and supports pause/resume/rate-limit hooks.

### Retry controller
Classifies transient errors and computes retry attempts.

## State Model
Queued -> Downloading -> Paused -> Downloading -> Completed
Queued/Downloading/Paused -> Canceled
Downloading -> Failed (after retry exhaustion)

## Operation Flows

### New download
1. Validate payload.
2. Create download state entry.
3. Enqueue and return downloadId.
4. Scheduler activates transfer when eligible.

### Pause/Resume
1. Resolve downloadId.
2. Validate state transition.
3. Update state and transport behavior.

### Retry
1. Receive transfer failure.
2. Classify transient/non-transient.
3. Requeue if retry budget remains.
4. Mark terminal failed otherwise.

## Concurrency Model
- State map is synchronized.
- Worker and control API coordinate through guarded transitions.
- Progress notifications avoid lock contention with transfer loop.

## Error Handling
- Unknown downloadId returns NOT_FOUND.
- Invalid control transition returns INVALID_STATE.
- Transport failures map to TRANSIENT_FAILURE or DEPENDENCY_UNAVAILABLE.

## Observability
- Logs include downloadId, URL hash/source, status, and failure category.
- Metrics include queue wait time, transfer duration, retry counts, and completion rates.
