# DownloadManager Detailed Requirements

## Scope
DownloadManager provides queue-based download scheduling, pause/resume/cancel controls, progress reporting, bandwidth limiting, and retry logic.

## Functional Requirements

### FR-DM-001: Queue and schedule downloads
The system SHALL accept download requests into a managed queue and execute according to scheduler policy.

Acceptance criteria:
- Request returns a unique downloadId.
- Invalid payload returns INVALID_ARGUMENT.
- Queue insertion failure returns INTERNAL_ERROR.

### FR-DM-002: Pause, resume, cancel controls
The system SHALL provide control operations for active and queued downloads.

Acceptance criteria:
- Pause transitions active download to paused.
- Resume continues paused transfer where supported.
- Cancel removes queued item or stops active transfer.

### FR-DM-003: Progress and status reporting
The system SHALL report status and transferred byte progress.

Acceptance criteria:
- Progress reflects current transferred and total bytes when known.
- Unknown downloadId returns NOT_FOUND.

### FR-DM-004: Rate limiting
The system SHALL allow setting bandwidth limits on download operations.

Acceptance criteria:
- Valid limits are applied to active transfers.
- Invalid limit values return INVALID_ARGUMENT.

### FR-DM-005: Retry transient failures
The system SHALL retry transient failures up to configured retry budget.

Acceptance criteria:
- Transient network errors trigger retries with policy.
- Retry exhaustion transitions download to terminal failed status.

## Non-Functional Requirements

### NFR-DM-001: Fairness
Queue policy SHALL be deterministic and starvation-resistant for supported priorities.

### NFR-DM-002: Resource bounds
Downloader worker threads and buffers SHALL remain within configured limits.

### NFR-DM-003: Resilience
Failure of one download SHALL not terminate unrelated queued/download operations.

## Failure Taxonomy Mapping
DownloadManager SHALL map failures to canonical categories:
- INVALID_ARGUMENT
- NOT_FOUND
- INVALID_STATE
- DEPENDENCY_UNAVAILABLE
- UNSUPPORTED_OPERATION
- TIMEOUT
- PERMISSION_DENIED
- TRANSIENT_FAILURE
- INTERNAL_ERROR
