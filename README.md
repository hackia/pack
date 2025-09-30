# pack

[![test](https://github.com/hackia/pack/actions/workflows/test.yml/badge.svg)](https://github.com/hackia/pack/actions/workflows/test.yml)

```shell
dd if=/dev/urandom of=rand.bin bs=1M count=50
pack encode rand.bin /tmp/input.hex
pack decode /tmp/input.hex output.bin
pack verify rand.bin /tmp/input.hex
```
