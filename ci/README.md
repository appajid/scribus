# Cross-platform validation

Scribus fork changes target three primary desktop configurations:

- Windows x64 with Visual Studio 2022 and Qt 6
- Linux x64 with GCC and Qt 6
- macOS ARM64 with Clang and Qt 6

The `cross-platform-portability.yml` workflow compiles and runs the isolated,
platform-neutral tests on all three configurations. This gate is intentionally
small so it gives quick feedback for core logic without requiring every Scribus
packaging dependency.

Run the same gate locally from the repository root with:

```sh
cmake -S ci/portable -B build/portable -DCMAKE_BUILD_TYPE=Release
cmake --build build/portable --config Release --parallel 4
ctest --test-dir build/portable -C Release --output-on-failure
```

## Release validation levels

1. **Portable logic:** the GitHub Actions matrix must pass on Windows x64,
   Linux x64, and macOS ARM64.
2. **Full application build:** build Scribus with its complete dependency set on
   each platform. The checked-in Visual Studio projects remain part of the
   Windows build contract and must include every application source.
3. **Integration tests:** run the installed-application tests, including SLA
   persistence, Scripter, dynamic variables, running headers, cross-references,
   and anchored objects.
4. **GUI smoke testing:** manually verify startup, document creation, themes,
   typography/font fallback, image import, PDF output, printing, and HiDPI on
   representative machines.

A feature is not considered certified for a platform merely because its
portable tests pass. Release certification requires the full build and relevant
integration and GUI checks on that platform.
