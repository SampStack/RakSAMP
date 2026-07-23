# Protocols

| Name | Network version | Default MTU | Client version string |
|---|---:|---:|---|
| SA-MP 0.3.7 | 4057 | 576 | `0.3.7` |
| SA-MP 0.3DL | 4062 | 1500 | `0.3.DL-R1` |

The client applies the selected network version to challenge handling, joining, and MTU configuration. It reads the 0.3DL-specific class, spawn, player-skin, and stream-in fields.

For custom models, the headless client consumes metadata and completes the download handshake but intentionally does not request or store DFF/TXD artwork. Gameplay automation therefore uses base model IDs.

The server records a protocol for every connected player and writes version-specific class, spawn, skin, and player stream layouts for the recipient. Both client families may connect concurrently.

The server does not host models or provide HTTP asset delivery. Use open.mp when testing the complete custom-artwork pipeline.

The open.mp 4057/4062 legacy-network implementation is the compatibility reference. Compatibility with original closed SA-MP binaries is checked manually where those binaries are legally available.
