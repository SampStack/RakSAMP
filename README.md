# RakSAMP

[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)](CMakeLists.txt)
[![SA-MP 0.3.7](https://img.shields.io/badge/SA--MP-0.3.7-2ea44f)](docs/protocols.md)
[![SA-MP 0.3DL](https://img.shields.io/badge/SA--MP-0.3DL-2ea44f)](docs/protocols.md)
[![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)](docs/development.md)
[![Architectures](https://img.shields.io/badge/arch-x64%20%7C%20ARM64-blue)](docs/development.md)
[![Containers](https://img.shields.io/badge/GHCR-public-informational?logo=github)](docs/releasing.md)
[![Native builds](https://img.shields.io/badge/native-downloads-public-informational?logo=github)](https://github.com/SampStack/RakSAMP/releases/tag/dev)
[![Publish](https://img.shields.io/badge/workflow-manual%20only-orange?logo=githubactions)](.github/workflows/publish-containers.yml)

RakSAMP provides two command-line tools for SA-MP development:

- **`raksamp-client`** — a headless client for scripted gamemode testing.
- **`raksamp-server`** — a Lua-scriptable protocol fixture for client development.

> [!IMPORTANT]
> Both products support **SA-MP 0.3.7 and 0.3DL**. New client configurations default to 0.3DL. The development server accepts both versions at the same time.

> [!WARNING]
> RakSAMP includes its historical load, flood, lag, and malformed-state commands. They are retained for debugging and resilience testing on systems you own or control. Nothing invokes them automatically.

## Contents

- [Five-minute client start](#five-minute-client-start)
- [Five-minute server start](#five-minute-server-start)
- [Downloads](#downloads)
- [Support matrix](#support-matrix)
- [Client commands](#client-commands)
- [Documentation](#documentation)
- [Project status](#project-status)

## Five-minute client start

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cp client/bin/RakSAMPClient.xml .
./build/bin/raksamp-client --config RakSAMPClient.xml --check-config
./build/bin/raksamp-client --config RakSAMPClient.xml
```

Set the server, nickname, and password in `RakSAMPClient.xml`. Override the configured protocol without editing the file:

```bash
./build/bin/raksamp-client --config RakSAMPClient.xml --protocol 0.3.7
```

Run one isolated lightweight client worker for a controlled load test:

```bash
RAKSAMP_LOAD_PASSWORD='testpassword' \
  ./build/bin/raksamp-client --config RakSAMPClient.xml \
  --load-clients 1 --load-duration 30 --load-connect-rate 5 \
  --load-account-prefix loadtest --load-character-first Load
```

Load mode expects those numbered accounts and matching characters to exist. It
ramps connections, completes login and textdraw character selection, sends
normal sync, and exits nonzero if any client fails or disconnects. Use it only
against servers you own or are authorized to test.

The mode deliberately runs one client per process because the legacy RakNet
implementation does not fully isolate peer state at higher in-process
concurrency. Harnesses coordinate workers with unique
`--load-index-offset` values and a shared `--load-start-file`, up to 100 clients
in total. An optional
`--load-anticheat-probe-clients N` flag makes the first N clients report
impossible movement, boosted vitals, and an unauthorized weapon during the
soak so an authorized server harness can verify its correction paths.

Public container:

```bash
docker run --rm -it \
  -v "$PWD/RakSAMPClient.xml:/work/RakSAMPClient.xml:ro" \
  ghcr.io/sampstack/raksamp-client:dev
```

## Five-minute server start

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cp -R server/bin/. work/
./build/bin/raksamp-server --config work/RakSAMPServer.xml --check-config
./build/bin/raksamp-server --config work/RakSAMPServer.xml
```

Public container:

```bash
docker run --rm -it -p 7777:7777/udp \
  -v "$PWD/work:/work" \
  ghcr.io/sampstack/raksamp-server:dev
```

> [!CAUTION]
> `raksamp-server` is a local and CI development tool, not a production game server. Lua scripts have the full Lua 5.4 standard library, including filesystem, process, and dynamic-loading access.

## Downloads

The manual release workflow publishes both delivery formats:

- **Native archives** — run without Docker on Windows, Linux, or macOS, on x64 or ARM64.
- **Containers** — public `linux/amd64` and `linux/arm64` images on GHCR.

Download the rolling [development release](https://github.com/SampStack/RakSAMP/releases/tag/dev), extract the archive for your product and platform, then run the executable beside its included XML configuration. Development assets are replaced only when a maintainer starts the manual workflow.

macOS archives are currently unsigned. If Gatekeeper quarantines a build you trust, remove the quarantine attribute from the extracted directory:

```bash
xattr -dr com.apple.quarantine raksamp-client
```

## Support matrix

| Capability | Client | Server |
|---|:---:|:---:|
| SA-MP 0.3.7 / network 4057 | ✅ | ✅ |
| SA-MP 0.3DL / network 4062 | ✅ | ✅ |
| Mixed 0.3.7 and 0.3DL sessions | N/A | ✅ |
| Headless stdin automation | ✅ | N/A |
| Process-isolated load worker | ✅ | N/A |
| Lua 5.4.8 scripting | N/A | ✅ |
| Custom-model metadata handshake | ✅ | N/A |
| DFF/TXD asset storage or hosting | ❌ | ❌ |
| Windows, Linux, macOS | ✅ | ✅ |
| x64 and ARM64 | ✅ | ✅ |
| Linux amd64/arm64 containers | ✅ | ✅ |

The maintained products are command-line only. `RakSAMP.slnx` provides a modern
Visual Studio and MSBuild entry point backed by CMake. Historical Visual Studio
GUI projects remain as reference material and are not part of supported builds
or packages.

## Client commands

Normal input is sent as chat; input beginning with `/` is sent as a server command. Local commands begin with `!`.

<details>
<summary>Automation and inspection commands</summary>

`!exit`, `!quit`, `!reconnect`, `!reload`, `!runmode`, `!players`, `!npcs`,
`!goto`, `!gotocp`, `!autogotocp`, `!spawn`, `!class`, `!pickup`, `!weapon`,
`!pos`, `!follow`, `!selplayer`, `!selveh`, `!vlist`, `!dialogresponse`,
`!menusel`, `!seltd`, `!sendrates`, `!log`, `!logstatus`, `!teleport`,
`!change_name`, `!change_server`, `!imitate`, and `!scmevent`.

</details>

<details>
<summary>Stress and resilience commands</summary>

`!lag`, `!spam`, `!joinflood`, `!chatflood`, `!classflood`, `!bulletflood`,
`!kill`, `!fakekick`, `!fu`, `!pulsator`, and `!vdeath`.

These commands preserve legacy behavior for controlled server debugging. They are never enabled by samples, tests, startup configuration, or container defaults.

</details>

## Documentation

- [Getting started](docs/getting-started.md)
- [Configuration](docs/configuration.md)
- [Protocols](docs/protocols.md)
- [Server scripting](docs/server-scripting.md)
- [Development](docs/development.md)
- [Releasing](docs/releasing.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)

## Project status

This fork modernizes the original RakSAMP codebase while preserving its established command and Lua APIs. The open.mp legacy-network implementation is the compatibility reference. Original SA-MP binaries are tested on a best-effort basis where available.

No repository-wide license has been added because the original project did not include one and relicensing authorization has not been established. See [third-party notices](THIRD_PARTY_NOTICES.md).
