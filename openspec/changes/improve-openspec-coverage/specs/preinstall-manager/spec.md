## ADDED Requirements

### Requirement: Spec traceability comments in all PreinstallManager source files
All production `.cpp` and `.h` files in the PreinstallManager module SHALL contain a `// Spec: preinstall-manager` comment at the top of the file to establish code-to-spec traceability.

#### Scenario: Source file contains spec reference
- **WHEN** any PreinstallManager `.cpp` or `.h` production file is opened
- **THEN** a `// Spec: preinstall-manager` comment is present at the top of the file

### Requirement: PreinstallManager spec completeness
The `openspec/specs/preinstall-manager/spec.md` SHALL contain all mandatory template sections.

#### Scenario: All required sections present
- **WHEN** the PreinstallManager spec.md is reviewed
- **THEN** all 13 mandatory sections are present and non-empty
