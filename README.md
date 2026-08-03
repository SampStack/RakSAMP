# RakSAMP

[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)](CMakeLists.txt)
[![SA-MP 0.3.7](https://img.shields.io/badge/SA--MP-0.3.7-2ea44f)](docs/protocols.md)
[![SA-MP 0.3DL](https://img.shields.io/badge/SA--MP-0.3DL-2ea44f)](docs/protocols.md)
[![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)](docs/development.md)
[![Architectures](https://img.shields.io/badge/arch-x64%20%7C%20ARM64-blue)](docs/development.md)
[![Containers](https://img.shields.io/badge/GHCR-public-informational?logo=github)](docs/releasing.md)
[![Native builds](https://img.shields.io/badge/native-downloads-public-informational?logo=github)](https://github.com/SampStack/RakSAMP/releases/tag/dev)
[![Publish](https://img.shields.io/badge/dev-manual-blue?logo=githubactions)](.github/workflows/publish-containers.yml)

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
RAKSAMP_LOAD_INPUT_RESPONSE='testpassword' \
  ./build/bin/raksamp-client --config RakSAMPClient.xml \
  --load-clients 1 --load-duration 30 --load-connect-rate 5 \
  --load-account-prefix loadtest --load-character-first Load \
  --load-player-name '{character}' --load-no-selection
```

Load mode expands `{account}`, `{character}`, and `{index}` in
`--load-player-name` and `--load-selection-text`. The defaults join as the
generated account and select the generated character textdraw. Single-identity
servers can choose another player-name template and `--load-no-selection`.
The first input/password dialog receives the configured input response. This
keeps the client independent of a gamemode's login model. Use it only against
servers you own or are authorized to test.

The mode deliberately runs one client per process because the legacy RakNet
implementation does not fully isolate peer state at higher in-process
concurrency. Harnesses coordinate workers with unique
`--load-index-offset` values and a shared `--load-start-file`, up to 100 clients
in total. Optional `--load-probe-*` flags let an external harness choose the
exact velocity, vitals, and weapon fields reported by selected workers. RakSAMP
does not assign meaning to those values; the implementing server and its test
harness define the expected response. The older
`--load-anticheat-probe-clients` spelling remains a compatibility alias.
`--load-password` and `RAKSAMP_LOAD_PASSWORD` remain compatibility aliases for
the generic input response.

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

The release workflow publishes both delivery formats:

- **Native archives** — Linux x64/ARM64, Windows x64, and macOS Intel/Apple Silicon.
- **Containers** — public `linux/amd64` images on GHCR.

Download the rolling [development release](https://github.com/SampStack/RakSAMP/releases/tag/dev),
extract the archive for your product and platform, then run the executable beside its included XML
configuration. Pushes and merges do not publish; the manual release workflow refreshes these
development assets by default.

macOS archives are ad-hoc signed. After verifying that the release and checksum are the ones you
intend to trust, authorize only the extracted package with its checksum-gated helper:

```bash
bash ./authorize-macos-runtime.sh --trust
```

## Support matrix

| Capability | Client | Server |
|---|:---:|:---:|
| SA-MP 0.3.7 / network 4057 | ✅ | ✅ |
| SA-MP 0.3DL / network 4062 | ✅ | ✅ |
| Mixed 0.3.7 and 0.3DL sessions | N/A | ✅ |
| Headless stdin automation | ✅ | N/A |
| Structured JSONL automation | ✅ | N/A |
| Process-isolated load worker | ✅ | N/A |
| Lua 5.4.8 scripting | N/A | ✅ |
| Custom-model metadata handshake | ✅ | N/A |
| DFF/TXD asset storage or hosting | ❌ | ❌ |
| Windows, Linux, macOS | ✅ | ✅ |
| Linux x64/ARM64, Windows x64, macOS x64/ARM64 | ✅ | ✅ |
| Linux amd64 containers | ✅ | ✅ |

The maintained products are command-line only. `RakSAMP.slnx` provides a modern
Visual Studio and MSBuild entry point backed by CMake. Historical Visual Studio
GUI projects remain as reference material and are not part of supported builds
or packages.

## Client commands

Pass `--jsonl` to use the versioned, language-neutral automation stream. Each
stdin line is either `{"command":"send","line":"/command"}` or
`{"command":"capabilities"}`. Stdout emits JSON objects, including log and
capability events. Plain-text stdin remains the default for compatibility.

Normal input is sent as chat; input beginning with `/` is sent as a server command. Local commands begin with `!`.

<details>
<summary>Automation and inspection commands</summary>

`!exit`, `!quit`, `!reconnect`, `!reload`, `!runmode`, `!players`, `!npcs`,
`!goto`, `!gotocp`, `!autogotocp`, `!spawn`, `!class`, `!pickup`, `!weapon`,
`!shoot`, `!shootmiss`, `!damage`, `!takedamage`, `!key`, `!pos`, `!follow`, `!selplayer`, `!selveh`, `!vlist`, `!dialogresponse`,
`!menusel`, `!seltd`, `!sendrates`, `!log`, `!logstatus`, `!teleport`,
`!change_name`, `!change_server`, `!imitate`, and `!scmevent`.

`!shoot <player> [weapon]` emits correlated bullet sync and damage RPCs.
`!shootmiss [weapon]` emits a non-damaging bullet miss for callback testing.
`!damage <player> <weapon> [amount]` emits a player damage RPC without
bullet sync, including for arbitrary valid server slots that are not streamed
to the client, covering melee and deliberate damage-before-shot ordering tests.
`!takedamage <issuer|-1> <cause> [amount]` emulates special or environmental
damage received by the headless client. The last command is explicit because
RakSAMP has no GTA physics engine to independently simulate explosions,
collisions, drowning, or falls; the server must still validate every report.

`!key <down|up> <action|mask> [action|mask ...]` holds or releases one or more
server-visible SA-MP controls and immediately emits the appropriate on-foot,
driver, or passenger sync. Named actions cover `action`, `crouch`/`horn`/`h`,
`fire`, `sprint`, `secondary_attack`, `jump`, `look_left`, `look_right`,
`handbrake`, `submission`/`look_behind`, `walk`/`lalt`, the four analog
directions, `yes`/`y`, `no`/`n`, and `ctrl_back`. Decimal and hexadecimal masks
are also accepted. These are protocol actions rather than operating-system
scancodes; Yes, No, and CtrlBack are mutually exclusive on the wire.

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
