# Pack



[![test](https://github.com/hackia/pack/actions/workflows/test.yml/badge.svg)](https://github.com/hackia/pack/actions/workflows/test.yml)

```shell
dd if=/dev/urandom of=rand.bin bs=1M count=50
pack encode rand.bin /tmp/input.hex
pack decode /tmp/input.hex output.bin
pack verify rand.bin /tmp/input.hex
```

## Usage

### Send a file

```shell
pack send README.md 192.168.1.6:8080
```

### Receive a file

```shell
pack recv 8080
```

### Send a directory

```shell
pack send . 192.168.1.6:8080
```

## Systemd 

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

## Systemd Service Installation

To install the systemd service, copy the unit file to `~/.local/share/systemd/user/pack-receive.service` and run:

```shell
mkdir -p ~/.local/share/systemd/user
install -m 644 pack-receive.service ~/.local/share/systemd/user/pack-receive.service
systemctl --user daemon-reload
systemctl --user enable pack-receive.service
systemctl --user start pack-receive.service
```
