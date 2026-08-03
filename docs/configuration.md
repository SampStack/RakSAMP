# Configuration

## Client

`RakSAMPClient.xml` retains the original settings and adds:

```xml
<RakSAMPClient protocol="0.3DL" clientversion="0.3.DL-R1" ...>
```

Accepted protocol values are `0.3.7` and `0.3DL`.

Selection order:

1. `--protocol` CLI override.
2. Explicit `protocol` XML attribute.
3. Missing `protocol` plus `clientversion="0.3.7..."` selects 0.3.7.
4. Otherwise 0.3DL.

Useful commands:

```bash
raksamp-client --config path/to/RakSAMPClient.xml --check-config
raksamp-client --config path/to/RakSAMPClient.xml --protocol 0.3.7
```

### Process-isolated load mode

Load mode is opt-in and runs one independent RakNet client worker per process.
Harnesses can use numbered login identities (`loadtest0001`, `loadtest0002`, …)
and independently template the name sent during connection and any selection
response required by the implementing server:

```bash
RAKSAMP_LOAD_INPUT_RESPONSE='testpassword' raksamp-client \
  --config path/to/RakSAMPClient.xml \
  --load-clients 1 \
  --load-duration 30 \
  --load-connect-rate 5 \
  --load-sync-rate 5 \
  --load-ready-timeout 180 \
  --load-probe-clients 0 \
  --load-account-prefix loadtest \
  --load-character-first Load \
  --load-player-name '{character}' \
  --load-no-selection
```

The generated accounts and characters must already exist. `--load-clients`
must be 1. Process isolation avoids cross-peer state leakage in the legacy
RakNet implementation. A load harness may shard up to 100 clients
across processes using non-overlapping `--load-index-offset` values and a common
`--load-start-file`; each process waits until the harness creates that file
before beginning its soak. The process reports aggregate readiness and fails if any
client cannot authenticate, satisfy its configured selection, spawn, or remain
connected through the soak. `--load-input-response` is also accepted, but the
environment variable avoids exposing credentials in process listings. The
older `--load-password` and `RAKSAMP_LOAD_PASSWORD` names remain compatibility
aliases.
When `--load-probe-clients` is nonzero, the first N clients emit the values set
by `--load-probe-health`, `--load-probe-armour`, `--load-probe-weapon`, and
`--load-probe-velocity-x` only after every client is active. This is an opt-in,
protocol-level state probe for servers you own. RakSAMP intentionally does not
encode what those values mean; the server-side harness defines and verifies the
expected behavior. The old `--load-anticheat-probe-clients` flag remains an alias.
`--load-player-name` and `--load-selection-text` accept `{account}`,
`{character}`, and `{index}` placeholders. Their defaults preserve the common
account-login plus character-textdraw flow. `--load-no-selection` supports
single-identity servers without teaching RakSAMP about a specific gamemode.

Do not place stress commands in `<autorun>` unless a controlled test explicitly requires them.

## Server

```xml
<server
  max_players="100"
  port="7777"
  name="Default RakSAMP server"
  password=""
  scripts="basic"
  lagcomp="1"
  protocols="0.3.7,0.3DL" />
```

The server currently requires both supported protocols. Configuration and scripts are resolved from the directory containing the selected XML file.

```bash
raksamp-server --config /work/RakSAMPServer.xml --check-config
```
