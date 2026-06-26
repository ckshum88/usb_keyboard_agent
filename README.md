# USB Keyboard Agent

This firmware is a minimal CherryUSB USB host example for Sipeed M0S Dock.  The M0S Dock which has a BL616 MCU is configured as an USB host which listens for a USB keyboard and forwards parsed key events to UART.

## Hareware

- Sipeed M0S Dock [Link to Sipeed](https://wiki.sipeed.com/hardware/en/maixzero/m0s/m0s.html)

| GPIO Pin | Function |
|----------|----------|
| GPIO 10 | UART1 TX |
| GPIO 12 | UART1 RX |

- USB OTG adapter

Since you cannot power the M0S Dock through its USB-C connector, you have to power it using the 5V pin or use a USB on-the-go PD adapter [[Link to Google Search]](https://share.google/EWdvzhuc13QQg9WHG). However, the M0S Dock does not do PD negotiation. You have to use a USB-A to USB-C cable.

## Build

- First you have to install the Bouffalo SDK [[Link to GitHub]](https://github.com/bouffalolab/bouffalo_sdk)
- Then, clone this repository and from the folder usb_keyboard_agent compile the code.

```sh
cd ~
git clone https://github.com/ckshum88/usb_keyboard_agent.git
cd usb_keyboard_agent
make clean && make CHIP=bl616 BOARD=bl616dk
```

- The default install path of Boufallo SDK is /opt/bouffalo_sdk. If you install your SDK to another location, edit Makefile accordingly.

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
