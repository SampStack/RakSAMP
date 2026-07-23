# Getting started

## Requirements

- CMake 3.24+
- A C++17 compiler
- Git and network access during the first configure (CMake downloads pinned Lua 5.4.8)

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Disable a product when iterating:

```bash
cmake -S . -B build-client -DRAKSAMP_BUILD_SERVER=OFF
cmake -S . -B build-server -DRAKSAMP_BUILD_CLIENT=OFF
```

Executables are written to `build/bin`.

## Client

```bash
./build/bin/raksamp-client \
  --config client/bin/RakSAMPClient.xml \
  --check-config

./build/bin/raksamp-client \
  --config client/bin/RakSAMPClient.xml
```

Commands are read from stdin. This works interactively and through pipes used by CI harnesses.

## Server

Keep the XML and `scripts/` directory together:

```bash
./build/bin/raksamp-server \
  --config server/bin/RakSAMPServer.xml \
  --check-config

./build/bin/raksamp-server \
  --config server/bin/RakSAMPServer.xml
```

Stop with `Ctrl+C` or `SIGTERM`.

## Containers

```bash
docker build --target client -t raksamp-client .
docker build --target server -t raksamp-server .
```

The runtime user is UID/GID `10001`. Ensure mounted files are readable and mounted directories are writable when logs or server state must persist.
