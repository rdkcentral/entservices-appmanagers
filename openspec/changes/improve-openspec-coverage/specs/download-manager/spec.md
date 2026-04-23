## ADDED Requirements

### Requirement: Spec traceability comments in all DownloadManager source files
All production `.cpp` and `.h` files in the DownloadManager module SHALL contain a `// Spec: download-manager` comment at the top of the file to establish code-to-spec traceability.

#### Scenario: Source file contains spec reference
- **WHEN** any DownloadManager `.cpp` or `.h` production file is opened
- **THEN** a `// Spec: download-manager` comment is present at the top of the file

### Requirement: DownloadManager spec completeness
The `openspec/specs/download-manager/spec.md` SHALL contain all mandatory template sections.

#### Scenario: All required sections present
- **WHEN** the DownloadManager spec.md is reviewed
- **THEN** all 13 mandatory sections are present and non-empty
