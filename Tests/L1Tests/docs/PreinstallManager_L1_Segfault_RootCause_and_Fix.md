# PreinstallManager L1 Segfault Root Cause and Resolution

## Verdict

The historical crash was caused by a test-side ABI/ODR mismatch in the former shared concrete runtime-configuration type, not by PreinstallManager business logic.

## Historical behavior

The same concrete type was independently declared in multiple public interface headers. Its layout depended on include order, so different translation units could build incompatible objects and destructor symbols. A worker-thread cleanup path could then select a destructor compiled for a different layout, producing a double free while clearing package information.

A temporary test-only canonical definition previously stabilized that layout and removed the reproducible crash.

## Current resolution

Runtime configuration now crosses the Package Manager, App Manager, Lifecycle Manager, and Runtime Manager boundaries as an opaque serialized JSON string. PreinstallManager treats the package-inspection output as an opaque string and does not construct or destroy a shared concrete configuration object.

The duplicated public structure and the test-local canonical definition have been removed. Runtime Manager alone decodes the payload into its private typed representation.

## Validation

1. PreinstallManager interface mocks use the opaque string output.
2. The package scan and force-install paths no longer instantiate the removed shared type.
3. Public interface headers no longer contain competing guarded structure definitions.
4. The existing PreinstallManager L0/L1 tests remain responsible for package scan, filtering, installation, failure, and worker cleanup behavior.
