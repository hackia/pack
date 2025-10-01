# Pack

A minimal, fast file transfer utility and protocol with optional integrity and authenticity verification.

[![test](https://github.com/hackia/pack/actions/workflows/test.yml/badge.svg)](https://github.com/hackia/pack/actions/workflows/test.yml)

- Language: C++20
- Build system: CMake (>= 3.31.6)
- Package discovery: pkg-config
- Crypto/hash libs: libsodium (Ed25519), BLAKE3
- Tests: GoogleTest (fetched via FetchContent)

## Overview

Pack provides:

- Hex encoding/decoding of files for transport or inspection
- BLAKE3 hashing of files
- TCP-based file transfer (sender and receiver)
- Optional integrity and authenticity verification: sender signs the file hash with Ed25519; receiver verifies the
  signature
- A simple CLI: `pack`

Security notes:

- Key pair is generated and stored under `~/.pack/` as `id_ed25519` (private) and `id_ed25519.pub` (public).
- Keep your private key secret. Share only `id_ed25519.pub` with peers that should verify your files.

## Requirements

- A C++20 compiler (e.g., GCC 13+/Clang 16+)
- CMake >= 3.31.6
- pkg-config
- Development packages for:
    - libsodium (for Ed25519 signing/verification)
    - libblake3 (for hashing)
- A POSIX-like environment (Linux/macOS). Networking is implemented using BSD sockets.

On Debian/Ubuntu, for example:

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libsodium-dev libblake3-dev
```

## Build

This project uses CMake and provides several targets.

Common targets:

- pack (executable): the CLI entry point
- k (library): core Pack functionality
- key (library): key management
- sync (library): simple TCP helper
- pack_test (executable): unit tests

Build with the active CMake profile (example with Release):

```sh
cmake --build cmake-build-release --target pack
```

Build all including tests:

```sh
cmake --build cmake-build-release --target pack pack_test
```

Install (optional):

```sh
cmake --build cmake-build-release --target install
```

This installs `pack` to the configured prefix’s `bin/` and headers/libs to `include/` and `libs/` respectively (per
CMakeLists.txt install rules).

## Usage

Quick demo:

```sh
# Create sample data
dd if=/dev/urandom of=rand.bin bs=1M count=50

# Hex encode/decode and verify
pack encode rand.bin /tmp/input.hex
pack decode /tmp/input.hex output.bin
pack verify rand.bin /tmp/input.hex
```

### CLI commands

Show help:

```sh
pack help
```

Generate a key pair (Ed25519) under ~/.pack/:

```sh
pack keygen
```

Send a file to a host:port:

```sh
pack send README.md 192.168.1.6:8080
```

Send a directory (recursively). A `.packignore` file in the directory can list patterns to skip (simple substring
match):

```sh
pack send . 192.168.1.6:8080
```

Receive files on a port (runs continuously and verifies signatures):

```sh
pack recv 8080
```

Other commands:

- `pack version` — prints version
- `pack list` — TODO: not implemented yet
- `pack delete <file> <host> <port>` — client-side command exists, but the server protocol/handler is not implemented;
  consider this experimental/TODO

Notes:

- The default network timeout is 30 seconds.
- When receiving, the server writes files as `<original_name>_YYYY-MM-DD<ext>` and verifies the Ed25519 signature
  against the received content.

## Environment variables

- HOME: used to resolve the key directory `~/.pack/`.

## Tests

Unit tests are built into the `pack_test` target and cover file existence, hashing, copying, and hex conversion.

Build and run tests (Release profile example):

```sh
cmake --build cmake-build-release --target pack_test && ./cmake-build-release/pack_test
```

Alternatively, relying on CTest discovery, you can run via the produced test executable path in your build tree.

## Project structure

High-level layout:

- apps/pack/main.cpp — CLI entry point for `pack`
- libs/include/Pack.hpp, libs/src/Pack.cpp — core file ops, hashing, and network send/receive
- libs/include/KeyManager.hpp, libs/src/KeyManager.cpp — Ed25519 key generation, save/load
- libs/include/Sync.hpp, libs/src/Sync.cpp — simple TCP utilities
- tests/main_test.cpp — GoogleTest unit tests
- pack-receive.service — example systemd user service unit
- CMakeLists.txt — build configuration and install rules
- LICENSE — project license

## Systemd (user service example)

You can run the receiver as a user service on port 8080:

```unit file (systemd)
[Unit]
Description=Pack Receive Service
After=network.target

[Service]
Type=simple
WorkingDirectory=%h/Pack
ExecStart=/usr/local/bin/pack recv 8080
Restart=always
RestartSec=3

[Install]
WantedBy=default.target
```

### Systemd Service Installation

Copy the unit file and enable the service:

```sh
mkdir -p ~/.local/share/systemd/user
install -m 644 pack-receive.service ~/.local/share/systemd/user/pack-receive.service
systemctl --user daemon-reload
systemctl --user enable pack-receive.service
systemctl --user start pack-receive.service
```

Note: Adjust `ExecStart` if you didn’t install to `/usr/local/bin`. If running system-wide, use the system instance of
systemd and an appropriate unit location.

## License

This project is distributed under the terms of the LICENSE file in this repository.

## TODO

- Implement `pack list` functionality
- Define and implement a server-side handler for `pack delete` protocol (DELETE <path> CRLF)
- Expand tests to cover networking paths and signature verification
