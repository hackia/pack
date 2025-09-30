# pack

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