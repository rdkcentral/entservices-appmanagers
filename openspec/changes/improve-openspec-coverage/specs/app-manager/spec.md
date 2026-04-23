## ADDED Requirements

### Requirement: Spec traceability comments in all AppManager source files
All production `.cpp` and `.h` files in the AppManager module SHALL contain a `// Spec: app-manager` comment at the top of the file to establish code-to-spec traceability.

#### Scenario: Source file contains spec reference
- **WHEN** any AppManager `.cpp` or `.h` production file is opened
- **THEN** a `// Spec: app-manager` comment is present at the top of the file

### Requirement: AppManager spec completeness
The `openspec/specs/app-manager/spec.md` SHALL contain all mandatory template sections: Overview, Description, Requirements, Architecture/Design, External Interfaces, Performance, Security, Versioning & Compatibility, Conformance Testing & Validation, Covered Code, Open Queries, References, and Change History.

#### Scenario: All required sections present
- **WHEN** the AppManager spec.md is reviewed
- **THEN** all 13 mandatory sections are present and non-empty
