## ADDED Requirements

### Requirement: Spec traceability comments in all LifecycleManager source files
All production `.cpp` and `.h` files in the LifecycleManager module SHALL contain a `// Spec: lifecycle-manager` comment at the top of the file to establish code-to-spec traceability.

#### Scenario: Source file contains spec reference
- **WHEN** any LifecycleManager `.cpp` or `.h` production file is opened
- **THEN** a `// Spec: lifecycle-manager` comment is present at the top of the file

### Requirement: LifecycleManager spec completeness
The `openspec/specs/lifecycle-manager/spec.md` SHALL contain all mandatory template sections.

#### Scenario: All required sections present
- **WHEN** the LifecycleManager spec.md is reviewed
- **THEN** all 13 mandatory sections are present and non-empty
