# RALF Support – Documentation Set

This folder documents, end to end, how RALF ("Bolt") package support was added to the
EntOS App Managers stack: from the libralf calls in `libpackage-sky`, through the
Thunder `PackageManager`, to the container launch paths in `RuntimeManager`.

| # | Document | What it answers |
|---|----------|-----------------|
| 1 | [01_RALF_Overview_and_Architecture.md](01_RALF_Overview_and_Architecture.md) | What RALF is, which repos/components take part, the data contract between them |
| 2 | [02_libpackage-sky_RALF_Changes.md](02_libpackage-sky_RALF_Changes.md) | **libpackage-sky changes in detail** – every new class, method, file and shared-code edit |
| 3 | [03_entservices-appmanagers_RALF_Changes.md](03_entservices-appmanagers_RALF_Changes.md) | PackageManager / RuntimeManager / AppStorageManager / AppManager changes |
| 4 | [04_Sequence_Diagrams.md](04_Sequence_Diagrams.md) | Boot, install, lock, launch, terminate, unlock, upgrade, uninstall sequences |
| 5 | [05_Launch_Modes.md](05_Launch_Modes.md) | The seven launch modes (ZIP / ZIP+ZIP runtime / ZIP+RALF runtime / ZIP on RALF runtime+base (OCI) / base-free RALF / OCI-bundle RALF / disabled) |
| 6 | [06_EntOS_Ifdefs_and_NonEntOS_Protection.md](06_EntOS_Ifdefs_and_NonEntOS_Protection.md) | Every compile-time gate, what it protects, and what is *not* protected for non-EntOS RALF users |
| 7 | [07_Troubleshooting_and_Log_Markers.md](07_Troubleshooting_and_Log_Markers.md) | Log lines per stage, on-device verification commands, known limits |
| 8 | [08_Remaining_Work_vs_Legacy_Appserviced.md](08_Remaining_Work_vs_Legacy_Appserviced.md) | What is still missing vs legacy appserviced (AI1), prioritised |
| 9 | [09_RALF_Management_and_Launch_AI2_vs_Legacy.md](09_RALF_Management_and_Launch_AI2_vs_Legacy.md) | How RALF package management and launch differ between AI2 and legacy appserviced |
| – | [ralf_support.html](ralf_support.html) | Single-page HTML version (rendered diagrams) – open in a browser |

## Source baselines used

| Repo | Branch | Baseline compared against |
|------|--------|---------------------------|
| `entservices-appmanagers` | `topic/RDKEMW-25785` | `develop` @ `9815a6a37` |
| `ref/libpackage-sky` | `topic/entos-ralf-support` @ `01d0ec6` | `develop` @ `55652dc` (tag `1.2.0.0`) |
| `ref/ralf-utils` | tag `v1.2.0` | – (external dependency, libralf) |
| `ref/legacyappserviced` | `main` | reference implementation (AI1 / appserviced) |

## One-paragraph summary

A RALF package is a signed archive whose payload is an EROFS image protected by dm-verity.
`libpackage-sky` detects RALF by **content** (`ralf::Package::detectPackageFormat`), verifies it
with the same CA roots as EntOS widgets, stores it in the **same** package slot as ZIP widgets,
registers converted metadata in the **same** `PackageDatabase`, and at lock time **mounts**
it read-only through libralf (`Package::mount(ReadOnly|DirectIO|NoSuid)`). The lock produces a
small JSON *mount manifest* whose path travels as `ralfPkgPath` through `PackageManager` to
`RuntimeManager`. `RuntimeManager` then chooses between a **classic Dobby spec** launch (no base
layer) and an **OCI bundle** launch (overlayfs of base/runtime/app layers + vendor GPU layer).
A ZIP widget app may also use a **RALF runtime** (e.g. YouTube widget + `com.sky.cobalt` RALF):
the runtime is mounted by libralf and bind-mounted at `/runtime` in the classic spec.
