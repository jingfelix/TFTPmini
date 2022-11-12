# TFTPmini

A simple tftp client for small-memory devices.

## Usage

```bash
Usage: tftp get <filename> <ip> <mode>
       tftp put <filename> <ip> <mode>
- filename: the file to be transfered or downloaded
- ip: the ip address of the server
- mode: netascii, octet
```

## Build

Debug mode:

```bash
make debug
```

Release mode:

```bash
make build
```

## More

> This also serves as 华中科技大学网安学院计算机网络实验2022

I do build a simple tftp server for debug for debug. But as it's not fully tested, I don't recommend to use it.
