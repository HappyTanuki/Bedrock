# _BedrockBuildConfig API

`cmake/_BedrockBuildConfig` is a config-only CMake package. It creates independently selectable INTERFACE policy targets under the exact `_BedrockBuildConfig::` namespace and defines narrow helper functions for target properties that cannot propagate through INTERFACE targets. It does not call `project()`, enable languages or tests, initialize submodules, fetch dependencies, create a `Bedrock` or component target, or change directory/global compile and link options. Every module uses `include_guard(GLOBAL)`, so adding the same directory again is a no-op.

## Consuming the package

A standalone component consumes only this subdirectory, without the Bedrock product or its submodules:

```cmake
include(FetchContent)
FetchContent_Declare(_BedrockBuildConfig
    GIT_REPOSITORY https://github.com/HappyTanuki/Bedrock.git
    GIT_TAG <full commit SHA that contains cmake/_BedrockBuildConfig>
    GIT_SUBMODULES ""
    GIT_SUBMODULES_RECURSE FALSE
    SOURCE_SUBDIR cmake/_BedrockBuildConfig)
FetchContent_MakeAvailable(_BedrockBuildConfig)
```

A pin is usable only if `git -C <Bedrock> show <sha>:cmake/_BedrockBuildConfig/CMakeLists.txt` succeeds; revisions created before this directory was committed do not contain the package.

To test unpublished provider changes, set `FETCHCONTENT_SOURCE_DIR__BEDROCKBUILDCONFIG` (double underscore, because the dependency name starts with `_`) to the Bedrock repository root, not to `cmake/_BedrockBuildConfig`. Check that `<root>/cmake/_BedrockBuildConfig/CMakeLists.txt` exists before configuring. A successful local override does not show that the pinned revision provides the package.

Common's `.cmake/build_config.cmake` selects the source in this order:

1. an explicit, non-empty `FETCHCONTENT_SOURCE_DIR__BEDROCKBUILDCONFIG`; it is never replaced, and a path without the package fails instead of falling back;
2. the parent Bedrock checkout that contains Common (`Common/..`);
3. a sibling `Bedrock` checkout next to Common;
4. the pinned `GIT_TAG`.

Discovered local paths (2 and 3) are not cached and are reported as `Using local Bedrock build config: <path>`. When target `Bedrock` already exists, Common uses the package loaded by the product instead. Readiness is checked through the real provider command `_BedrockBuildConfig_set_language_properties`.

## Targets

Link only the carriers a target needs. Keep warning carriers PRIVATE; use PUBLIC only for requirements consumers must share, such as the language level or a deliberate ISA choice. A carrier without an entry for the active toolchain is empty.

| Target | Usage requirement |
|---|---|
| `Cxx23` | compile feature `cxx_std_23` |
| `Warnings` | MSVC `/W4` plus the selected `/w1…` and `/we4289` warnings; GCC `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wundef -Wunreachable-code -Wstrict-aliasing -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wcast-qual -Wcast-align -Wuseless-cast`; Clang `-Weverything` with the `-Wno-…` exclusions listed in `Targets.cmake` |
| `WarningsAsErrors` | MSVC `/WX`; GCC and Clang `-Werror` |
| `MsvcParallel` | MSVC `/MP` |
| `Utf8` | MSVC `/utf-8` (UTF-8 source and execution character sets) |
| `MsvcConformance` | MSVC `/permissive- /Zc:__cplusplus /Zc:preprocessor` |
| `NoExceptions` | non-Windows `-fno-exceptions` |
| `NoRtti` | non-Windows `-fno-rtti` |
| `TestWarnings` | Clang-only warning exclusions for test code |
| `NativeIsa` | with `BEDROCK_NATIVE_ISA` (default ON): MSVC x86/x64 `/arch:${BEDROCK_MSVC_ARCH}` (default `AVX2`; empty disables it), GCC/Clang x86 `-march=native`, non-MSVC ARM `-mcpu=native` |
| `X86CryptoIsa` | non-Windows x86 `-maes -msse2 -mssse3` |
| `StaticMuslRuntime` | non-Windows link option `-static` |
| `StaticCxxRuntime` | non-Windows link options `-static-libgcc -static-libstdc++` |
| `ClangTidy` | none; `CXX_CLANG_TIDY` is a target property, so use `_BedrockBuildConfig_enable_clang_tidy` |

Runtime carriers only transport linker options; they do not select the product form or the MSVC CRT. Set `BEDROCK_NATIVE_ISA=OFF` for portable binary packages.

Every compilation closure needs its own carriers. Static test archives, test executables, helper DLLs that compile production sources directly, and third-party test libraries do not inherit `Cxx23` or `Utf8` from a production library they do not link. Without `/utf-8`, MSVC reads BOM-less UTF-8 sources in the system code page.

## Functions

```cmake
_BedrockBuildConfig_detect_architecture(OUT_PREFIX <prefix>)
_BedrockBuildConfig_detect_libc(OUT_VAR <variable>)
_BedrockBuildConfig_validate_linkage(LIBRARY_TYPE <STATIC|SHARED> LIBC <libc> LABEL <name>)
_BedrockBuildConfig_set_language_properties(<target>)
_BedrockBuildConfig_enable_pch(<target> HEADER <absolute-path>)
_BedrockBuildConfig_set_msvc_runtime(<target> VALUE <MSVC_RUNTIME_LIBRARY value>)
_BedrockBuildConfig_set_msvc_debug_information(<target>)
_BedrockBuildConfig_set_origin_rpath(<target>)
_BedrockBuildConfig_enable_windows_exports(<target>)
_BedrockBuildConfig_copy_runtime_dlls(<target>)
_BedrockBuildConfig_enable_clang_tidy(<target>)
_BedrockBuildConfig_stage_public_headers(SOURCE_ROOT <dir> DESTINATION <dir> [FILES <relative-paths...>] [TREES <relative-dirs...>] OUT_VAR <variable>)
_BedrockBuildConfig_assert_header_visibility(<target> SOURCE_ROOT <dir>)
_BedrockBuildConfig_copy_compile_commands(NAME <target> SOURCE <file> DESTINATION <file>)
_BedrockBuildConfig_get_targets(<out-variable>)
_BedrockBuildConfig_export_build_tree(DESTINATION <file>)
_BedrockBuildConfig_register_targets(INSTALL_DESTINATION <relative-package-dir>)
_BedrockBuildConfig_register_package(INSTALL_DESTINATION <relative-package-dir> EXPORT_NAME <export-set> NAMESPACE <namespace> CONFIG_FILE <config-file> TARGETS <targets...>)
```

- `detect_architecture` sets `<prefix>_ARCH_X86`, `<prefix>_ARCH_ARM` (0 or 1) and `<prefix>_ARCH_NAME` (`x86`, `arm`, or the raw `CMAKE_SYSTEM_PROCESSOR`) in the caller scope.
- `detect_libc` reports `glibc`, `musl` or `other` for native Linux builds and `unknown` otherwise. `validate_linkage` rejects STATIC linkage with glibc on Linux.
- `set_language_properties` sets `CXX_STANDARD 23`, `CXX_STANDARD_REQUIRED ON` and `CXX_EXTENSIONS OFF`.
- `enable_pch` adds a PRIVATE precompiled header. `set_msvc_runtime` sets `MSVC_RUNTIME_LIBRARY`. `set_msvc_debug_information` selects `ProgramDatabase` for Debug and RelWithDebInfo with MSVC.
- `set_origin_rpath` applies only to non-Apple Unix Release builds. It sets `$ORIGIN` as both build and install RPATH and builds with the install RPATH, so it assumes dependent shared libraries are placed next to the binary.
- `enable_windows_exports` sets `WINDOWS_EXPORT_ALL_SYMBOLS` on Windows. `copy_runtime_dlls` copies `$<TARGET_RUNTIME_DLLS>` next to the target after each Windows build.
- `enable_clang_tidy` sets `CXX_CLANG_TIDY` to report `google-readability-casting` when `clang-tidy` was found at configure time (`_BEDROCK_CLANG_TIDY_EXE`). It never passes `-fix`: rewriting sources during a build races between translation units and modifies source trees, including fetched ones.
- `stage_public_headers` deletes `DESTINATION`, copies the listed files and every file under the listed trees at configure time, and returns `DESTINATION` in `OUT_VAR`. `assert_header_visibility` fails if `SOURCE_ROOT` itself is an entry of the target's `INTERFACE_INCLUDE_DIRECTORIES`.
- `copy_compile_commands` creates an ALL target that copies the compile database. It does so only when `CMAKE_EXPORT_COMPILE_COMMANDS` is ON and the generator is single-config Ninja or a Unix, MinGW, MSYS or NMake Makefile generator; Visual Studio generators do not write `compile_commands.json`.
- `get_targets` returns the real carrier target names (`_BedrockBuildConfig_<Name>`). `export_build_tree` writes them to a build-tree export file in the `_BedrockBuildConfig::` namespace.
- `register_targets` installs every carrier in export set `_BedrockBuildConfigTargets` with the `_BedrockBuildConfig::` namespace, and installs `_BedrockBuildConfigConfig.cmake` generated from `_BedrockBuildConfigConfig.cmake.in`.
- `register_package` installs `TARGETS` in export set `EXPORT_NAME` under `NAMESPACE`, and installs `CONFIG_FILE` copied as `<EXPORT_NAME>Config.cmake`.

Including the package has no install side effects. `register_targets` and `register_package` share one per-configure guard: the first call installs, and any later call to either function does nothing.

## Presets

The preset fragments under `cmake/_BedrockBuildConfig/presets/` are canonical. CMake reads presets before configuring, and therefore before FetchContent runs, so consumers keep checked-in copies; Common includes them from `.cmake/presets/`. Run `python tools/sync_build_config_presets.py --destination <Common-root>` to update the copies, or add `--check` to report drift without writing. `--check` exits 1 on drift, and a missing canonical fragment exits 1 in both modes.

## Bedrock product composition

The Bedrock product root is the intended in-tree consumer: it adds `cmake/_BedrockBuildConfig` right after `project()` and before any component, so components see target `Bedrock` and reuse the loaded package. That root integration (component switches, source-directory overrides and target-scoped root policy) is separate follow-up work and is not part of this package; check the root `CMakeLists.txt` of the revision in use before relying on it. A root that creates target `Bedrock` without adding this directory first makes such a component fail at configure time with `Target Bedrock exists, but its build-config package is not loaded`.
