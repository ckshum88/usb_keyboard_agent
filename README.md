# USB Keyboard Agent

This firmware is a minimal CherryUSB USB host example for M0S Dock.  The M0S Dock which has a BL616 MCU is configured as an USB host which listens for a USB keyboard and forwards parsed key events to UART.

## Hareware

M0S Dock:
GPIO 16 - UART1 TX
GPIO 17 - UART1 RX

## Build

From this folder:

```sh
make CHIP=bl616 BOARD=bl616dk
```

## Flash

```sh
bflb-flash-command \
  --interface=uart \
  --baudrate=2000000 \
  --port=/dev/cu.usbmodem2211301 \
  --chipname=bl616 \
  --cpu_id= \
  --config=flash_prog_cfg.ini
```
