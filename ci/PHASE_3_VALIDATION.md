# Phase 3 validation status

Date: 2026-09-13

Validated source revision: `9b5293c3139829223e84f68558f53cfd9e0eaed7`

Phase 3 implementation is complete in the repository. Automated macOS ARM64
and Debian sid x86_64 validation passes. Release certification remains open
until the on-screen macOS walkthrough and a native Windows/MSVC build and GUI
smoke test are completed.

## Closure checklist

| Step | Result | Evidence / remaining gate |
|---|---|---|
| Atomic Object Style import undo | Complete | A single Undo removes imported styles and newly imported colours/line styles; Redo restores the complete import. Covered by the installed-application test. |
| macOS application validation | Automated checks complete | Native ARM64 Release build and all 17 tests pass. Offscreen UI smoke and canvas rendering pass. The Desktop test bundle is refreshed and ad-hoc signed. A human on-screen walkthrough is still required. |
| Windows validation | Portable checks complete | Six portable tests pass and the Visual Studio 2019/2022/2026 project-registration audit covers the Phase 3 UI and Scripter sources. Native MSVC compilation, installed integration tests, and GUI smoke require a Windows x64 runner. |
| Linux application validation | Complete for Debian sid x86_64 | Native full build, all 17 tests, clean package installation, ELF dependency scan, and installed dynamic-variable/Object-Style/anchored-object tests pass. |
| Final regression audit | Complete with external gates | No Phase 1-3 automated regressions found. The two manual/native gates above prevent a three-platform release-certified label today. |

## Validated platforms

### macOS ARM64

- Release configuration on macOS Tahoe, minimum deployment target macOS 12.
- Qt 6.11.1, Poppler 26.08.0, PoDoFo 1.1.2, Python 3.14.7.
- CMake configuration and the `Scribus` and `scriptplugin` targets pass.
- CTest: 17/17 passed.
- Prior offscreen application smoke covered the modern contextual toolbar,
  appearance switching, compact tool palette/flyouts, and canvas creation.
- Test bundle: `/Users/appajiambarishadarbha/Desktop/Scribus2.0.0.app`.

Manual release gate:

1. Launch the Desktop bundle normally (not offscreen).
2. Create, save, close, and reopen a document.
3. Exercise light, dark, and system appearance modes.
4. Check the tool palette, contextual controls, Properties Inspector, master
   pages, Quick Apply, Object Styles, cross-references, and anchored image/table
   editing.
5. Import an image and a PDF, then export a PDF and inspect Preflight.

### Debian forky/sid x86_64

- GCC 16.2.0, Qt 6.10.2, Poppler 26.07.0, ICU 78.3, Python 3.14.7.
- Full application and all enabled plugins: 1,266/1,266 build targets passed.
- CTest: 17/17 passed, including all nine tests labelled `phase-3`.
- Production rebuild uses `WITH_TESTS=OFF`; the installed executable has no
  `libQt6Test` dependency.
- A clean `debian:sid` container resolved and installed every declared package.
- The main executable and all installed plugins have no unresolved ELF
  dependencies.
- Installed-package smoke tests passed for dynamic variables, atomic Object
  Style import/undo, and anchored image/table reflow and persistence.

Package:

- File: `/Users/appajiambarishadarbha/Documents/Codex/scribus-linux-debian-sid-amd64/scribus-fork-test_2.0.0_debian-sid_amd64.deb`
- Size: 160,512,192 bytes.
- SHA-256: `e47fb5cac01ff3e69419f07ff5e9b98d4505e5451085c19100e7261691d82a29`
- Target: Debian forky/sid, x86_64 only. It is not an Ubuntu package.

The package metadata is generated from Debian sid's own shared-library
database. This replaces the obsolete Ubuntu-specific dependencies from the
earlier package (`libicu74`, `libjpeg8`, `libpoppler134`, Python 3.12 t64, and
the older Qt t64 package names). `qt6-qpa-plugins` is an explicit runtime
dependency so the application has usable Linux platform backends.

### Windows x64

- Portable CMake suite: 6/6 tests passed locally.
- Visual Studio source registration is checked for VS 2019, 2022, and 2026,
  including Object Style management/import UI and Scripter files.
- The GitHub Actions portability matrix is configured for Windows Server 2022
  with Qt 6.11.2.

Native release gate:

1. Configure and compile the complete application with MSVC x64 and Qt 6.
2. Run the installed-application integration suite.
3. Verify plugin loading and DLL discovery in a clean Windows environment.
4. Perform the same GUI workflow used for the macOS manual gate, including
   HiDPI and native file dialogs.

## Commits completing the audit

- `d96e161` Make Object Style imports atomically undoable.
- `6db98f0` Expand Windows project portability checks.
- `9b5293c` Fix Poppler discovery from pkg-config include paths.

The Poppler fix respects direct include paths returned by pkg-config. It was
verified with Debian's split `libpoppler-private-dev` headers and Homebrew's
Poppler layout.

## Findings and follow-up

- New GCC/Qt versions emit warnings in older Scribus and bundled third-party
  code. They did not fail the build or tests and were not mixed into Phase 3 as
  unrelated refactors. They should be handled in a dedicated warning-cleanup
  series with focused tests.
- The minimal clean Debian image only contains DejaVu fonts, so Qt reports that
  those fonts lack OpenType coverage for several Indic scripts. This is a font
  coverage warning, not evidence that the HarfBuzz/ICU shaping path failed.
  Indic typography still needs a dedicated visual regression suite using Noto
  Indic fonts before release certification.
- macOS on-screen behavior cannot be certified by headless automation, and a
  Linux container cannot substitute for the Windows ABI, MSVC, DLL loader, or
  native UI. Those two gates remain explicit rather than being inferred from
  portable tests.
