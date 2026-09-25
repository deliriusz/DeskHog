# Build, test, and release

DeskHog uses PlatformIO with one active environment.

## Environment

```ini
[env:adafruit_feather_esp32s3_reversetft]
```

The environment targets the Adafruit ESP32-S3 Reverse TFT Feather with Arduino, PSRAM, a custom OTA partition table, and release optimization. LVGL 9.2.2, the Adafruit ST7789 driver, Bounce2, ArduinoJson, FastLED, AsyncTCP, and ESPAsyncWebServer are installed through `lib_deps`.

Important build flags enable USB CDC, PSRAM-aware ArduinoJson allocation, the ESP32 PSRAM cache workaround, include paths, debug logging, and `CURRENT_FIRMWARE_VERSION`.

## Common commands

Build:

```sh
pio run -e adafruit_feather_esp32s3_reversetft
```

Upload:

```sh
pio run -e adafruit_feather_esp32s3_reversetft -t upload
```

Monitor:

```sh
pio device monitor --baud 115200
```

The configured upload protocol is `esptool` at 1,500,000 baud with `--chip=esp32s3 --no-stub`.

## Pre-build generators

Every build runs the portal, font, and sprite generators and can modify tracked files under `include/`. [Generated assets](assets.md) owns their inputs, outputs, prerequisites, and failure behavior. Always review the generated diff.

## Tests

There is currently no enabled native test environment. `platformio.ini` contains only a commented example, and `test/README` is the generic PlatformIO placeholder.

Do not report that tests passed unless you first configure and run an actual test target. For present changes, validation generally consists of:

- a successful firmware build;
- targeted serial-log inspection;
- hardware behavior on the actual ESP32-S3 board;
- repeated lifecycle tests for dynamic cards;
- heap/PSRAM and firmware-size review for large data/assets.

Parser and pure game-model code are candidates for a future native test environment, but Arduino/LVGL dependencies need to be isolated or stubbed deliberately.

The mutable card-request JSON boundary has a focused host regression that uses
the ArduinoJson dependency installed by the firmware build:

```sh
python3 tools/test_json_envelope.py
```

## Hardware validation matrix

For UI/card changes:

- boot with clean NVS and existing NVS;
- add, remove, reorder, and revisit cards;
- navigate and exercise center-button behavior;
- run while Wi-Fi reconnects and insight data arrives.

For portal/network changes:

- first-boot AP and captive-portal discovery;
- scan, save credentials, timeout/failure, and successful IP acquisition;
- portal access after station connection;
- malformed, duplicate, and queue-full requests.

For OTA changes:

- update check with and without Wi-Fi;
- no-update, malformed-release, missing-asset, and available-update cases;
- firmware size within the OTA slot;
- successful update and serial-flash recovery.

## Release workflow

`.github/workflows/build-release.yml` runs for a created GitHub release or manual dispatch. It:

1. installs PlatformIO, Pillow, and NumPy;
2. derives the version and updates `CURRENT_FIRMWARE_VERSION`;
3. builds the configured environment;
4. verifies `.pio/build/.../firmware.bin`;
5. packages stable and versioned firmware plus ELF, partitions, bootloader, and a flash command;
6. publishes artifacts and, for releases, attaches them to GitHub.

The stable OTA asset contract is maintained in [OTA updates](ota.md#release-requirements).

### Flash-command warning

Do not use the current workflow-generated `flash-command.txt`: it writes application firmware at `0x10000`, while `partitions.csv` places `ota_0` at `0x20000`. The workflow must use the partition-table offset. OTA installation is unaffected because the Arduino `Update` library selects the inactive application partition.

## Flash and recovery

If a development upload leaves the board booting an unexpected OTA slot, use PlatformIO's **Erase Flash and Upload** task and provision again. See [Hardware and power](hardware-and-power.md#reset-and-bootloader-recovery) for the ROM-bootloader sequence.

The normal mode in `MULTI_FLASH_GUIDE.md` uses PlatformIO-managed uploads. Do not use `multi_flash.py -f`: it writes an application-only image at bootloader offset `0x0`.
