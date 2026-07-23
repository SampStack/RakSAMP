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

Do not place stress commands in `<autorun>` unless a controlled test explicitly requires them.

## Server

```xml
<server
  max_players="1000"
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
