# Development

## Supported build

CMake is the supported build system. The old Visual Studio projects are
historical references only.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Visual Studio 2022 and modern MSBuild users can open `RakSAMP.slnx`. The
solution exposes Debug and Release configurations under Any CPU, then delegates
configuration and compilation to CMake so the active native toolchain still
selects the actual target architecture.

```bash
dotnet build RakSAMP.slnx --configuration Release --no-restore
dotnet msbuild RakSAMP.slnx -t:Test -p:Configuration=Release
```

Native targets are Linux x64/ARM64, Windows x64, and macOS x64/ARM64. Containers intentionally
target only Linux amd64. Manually run `Build and publish releases` with `build_only` selected to
validate and retain all five targets without publishing.

## Layout

- `client/src` — headless client and stdin driver
- `server/src` — development server and Lua bindings
- `common` — protocol structures and shared definitions
- `raknet`, `tinyxml` — preserved third-party legacy dependencies
- `tests` — non-network unit and fixture tests

## Compatibility work

Keep wire structures fixed-width and explicitly packed only where the protocol requires it. Never serialize native pointers, `long`, or compiler-dependent layout. Any protocol-specific outgoing RPC must be serialized for its recipient rather than broadcast using the sender's layout.

Stress-command tests must exercise parsing/state transitions without sending traffic outside an explicitly started loopback fixture.

## Before review

```bash
cmake --build build --parallel
ctest --test-dir build --output-on-failure
build/bin/raksamp-client --config client/bin/RakSAMPClient.xml --check-config
build/bin/raksamp-server --config server/bin/RakSAMPServer.xml --check-config
```

Run the same supported paths under AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake -S . -B build-sanitized -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON -DRAKSAMP_ENABLE_SANITIZERS=ON
cmake --build build-sanitized --parallel
ctest --test-dir build-sanitized --output-on-failure
```
