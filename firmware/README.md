# AirObserver

TODO: Main description

## Project Setup

Run `setup.sh` from this directory to create the Python and Zephyr pre-requisites.
This will also download all other project submodules, and install the local SDK.

Actiate the workspace venv via: `source .venv/bin/activate`.

## Building

Building for target is done via `west build -b <board> app -d build/app`.
See [Boards](#boards) below for supported targets.

## Flashing

Depends on board, refer below for instructions.

## Boards

### nRF52840 Dongle (PCA10059)

Flashes via Nordic's built-in USB DFU bootloader (`nrfutil`).

**Build:**

`west build -b nrf52840dongle/nrf52840 app -d build/app`

**Flash:**

1. Press RESET to activate bootloader mode, indicated by **LD2** pulsing red.
2. Package and flash:

```bash
nrfutil nrf5sdk-tools pkg generate \
    --hw-version 52 --sd-req=0x00 \
    --application build/app/zephyr/zephyr.hex \
    --application-version 1 \
    build/app/zephyr/app_dfu.zip

nrfutil nrf5sdk-tools dfu usb-serial -pkg build/app/zephyr/app_dfu.zip -p /dev/ttyACM0
```

Notes:
- Defalt by `west flash` expects J-link via `nrfutil` runner, so fails.

Console:
- USB CDC-ACM console via the USB-port @ 115200 8N1, e.g.: `picocom /dev/ttyACM0 -b 115200`.
