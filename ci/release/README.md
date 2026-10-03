# Apscribe 2.0.0 RC1 test installers

These packages are release candidates, not a stable release. Keep the `rc1`
suffix in published asset names and mark the GitHub release as a prerelease.
The repository tag provides the corresponding GPL source; bundled license and
copyright notices must remain in the installers.

| Platform | Candidate | Verification |
| --- | --- | --- |
| Windows x64 | NSIS `Setup.exe` from `Windows native package` | CI install/uninstall and packaged Phase 5 tests |
| Linux x86_64 | AppImage from `Linux AppImage candidate` | CI integration tests and AppImage version smoke |
| macOS ARM64 | unsigned DMG from `macOS DMG candidate` | local tests and bundle smoke; CI mount and smoke |

The current macOS candidate is built on macOS 27 with Homebrew dependencies
whose minimum versions vary. It is intended for macOS 27 ARM64 testing only;
rebuild against an older SDK and test there before claiming older macOS
support. It is ad-hoc signed but **not** Developer ID signed or notarized, so
Gatekeeper will block a normal public installation. Do not describe it as a
production-ready Mac installer.

The Linux AppImage is built on Ubuntu 24.04, so it is not guaranteed to run on
distributions with an older glibc. Test it on a clean target distribution,
including Debian sid, before publishing a stable release. The Windows NSIS
installer also requires a real Windows user acceptance test.

Check every `.sha256` file against its adjacent binary before upload. The
release tag and all three packaged binaries must come from the same source
revision. A draft release is appropriate until every installer is verified;
only then publish it as a prerelease. Do not promote to a stable `v2.0.0`
release until platform install, launch, document-open, and uninstall testing
are complete.
