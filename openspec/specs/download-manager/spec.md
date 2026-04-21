# Capability: Download Manager

## Requirements

### Requirement: Queue and execute downloads
The system SHALL queue download requests and execute them using supported network protocols.

#### Scenario: Queue download
Given a download request with URL and destination context
When the request is submitted
Then the system adds the request to its managed queue
And begins execution according to scheduler policy

#### Scenario: Queue rejects malformed request
Given request omits mandatory URL or destination input
When queue submission is attempted
Then the system returns invalid-parameter failure

### Requirement: Support pause, resume, and cancel controls
The system SHALL support pausing, resuming, and canceling individual downloads.

#### Scenario: Pause and resume
Given a download is in progress
When pause is requested and later resume is requested
Then transfer progress pauses and later continues from the most recent supported offset/state

#### Scenario: Cancel queued request
Given a download is queued and not yet started
When cancel is requested
Then the request is removed from queue and reported as canceled

### Requirement: Report download progress and status
The system SHALL provide observable status/progress updates for download operations.

#### Scenario: Progress observation
Given an active download
When progress is queried or emitted
Then the system reports current transferred size and completion state

#### Scenario: Progress request for unknown download id
Given no download matches the provided id
When progress is requested
Then the system returns not-found failure

### Requirement: Support configurable bandwidth control
The system SHALL allow rate limits to be set for download operations.

#### Scenario: Apply rate limit
Given an active or queued download
When a rate limit is set
Then transfer behavior honors the configured limit within runtime/network constraints

#### Scenario: Invalid rate limit rejected
Given rate limit is zero or outside accepted bounds
When rate limit is requested
Then the system returns invalid-parameter failure

### Requirement: Retry transient failures
The system SHALL retry eligible transient download failures according to configured retry behavior.

#### Scenario: Transient network failure retry
Given a download encounters a transient network error
When retry budget remains
Then the system retries the download before marking it failed

#### Scenario: Retry budget exhausted
Given transient failures continue until retry budget is exhausted
When final retry fails
Then download is marked failed with final error reason

### Requirement: Use standardized failure taxonomy
The system SHALL classify operation failures using canonical categories: INVALID_ARGUMENT, NOT_FOUND, INVALID_STATE, DEPENDENCY_UNAVAILABLE, UNSUPPORTED_OPERATION, TIMEOUT, PERMISSION_DENIED, TRANSIENT_FAILURE, and INTERNAL_ERROR.

#### Scenario: Consistent category mapping
Given a download operation fails
When error is returned
Then one canonical taxonomy category is present
And equivalent failures map consistently across queue/control APIs

#### Scenario: Preserve transport detail
Given network/transport layer returns native failure detail
When failure is propagated
Then canonical category is returned
And native detail is preserved as supplemental context
