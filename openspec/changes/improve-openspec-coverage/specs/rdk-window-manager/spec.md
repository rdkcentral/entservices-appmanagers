## ADDED Requirements

### Requirement: Spec traceability comments in all RDKWindowManager source files
All production `.cpp` and `.h` files in the RDKWindowManager module SHALL contain a `// Spec: rdk-window-manager` comment at the top of the file to establish code-to-spec traceability.

#### Scenario: Source file contains spec reference
- **WHEN** any RDKWindowManager `.cpp` or `.h` production file is opened
- **THEN** a `// Spec: rdk-window-manager` comment is present at the top of the file

### Requirement: RDKWindowManager spec completeness
The `openspec/specs/rdk-window-manager/spec.md` SHALL contain all mandatory template sections.

#### Scenario: All required sections present
- **WHEN** the RDKWindowManager spec.md is reviewed
- **THEN** all 13 mandatory sections are present and non-empty
