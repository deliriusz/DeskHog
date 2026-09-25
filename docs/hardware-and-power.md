# Hardware and power

DeskHog targets the Adafruit ESP32-S3 Reverse TFT Feather: dual-core ESP32-S3, integrated 240×135 ST7789 TFT, three buttons, PSRAM, and one NeoPixel.

Use board-provided names such as `TFT_CS`, `TFT_DC`, `TFT_RST`, `TFT_BACKLITE`, and `NEOPIXEL_POWER`. Do not duplicate pin values when the board variant already defines them. The installed variant's `pins_arduino.h` is the source of truth.

## Display

The board uses an SPI-connected ST7789 TFT with a 240×135 landscape UI. [Display and LVGL](display-and-lvgl.md) is the source of truth for driver initialization, buffers, backlight settings, and rendering ownership.

## PSRAM and heap

`setup()` calls `psramInit()` and halts permanently if initialization fails. It logs total/free PSRAM and enables external-memory allocation for allocations of at least 4096 bytes.

PSRAM is important for display buffers, TLS, and large ArduinoJson documents. Build flags enable PSRAM support and PSRAM-aware ArduinoJson behavior.

`sdkconfig.defaults` contains proposed mbedTLS settings, but `platformio.ini` assigns it to `board_build.embed_txtfiles`. That embeds the file as data instead of applying ESP-IDF configuration. Do not rely on those settings until the build configuration is corrected and verified.

When adding large buffers:

- check allocation failure;
- avoid per-frame allocation;
- log sizes, not sensitive contents;
- test simultaneous TLS, parsing, and rendering;
- remember that successful allocation does not guarantee adequate stack space.

## Buttons

| Button | GPIO | Mode | Pressed level |
|---|---:|---|---|
| Down / BOOT | 0 | Pull-up | LOW |
| Center | 1 | Pull-down | HIGH |
| Up | 2 | Pull-down | HIGH |

The table above is the hardware pin reference. Debouncing, input dispatch, navigation, and the center+down sleep chord are documented in [Input and navigation](input-and-navigation.md).

## Power management

Startup configures ESP32 power management with:

- maximum CPU frequency 240 MHz;
- minimum CPU frequency 10 MHz;
- automatic light sleep enabled.

All three button GPIOs are configured as wake sources for normal light sleep using their active levels.

For the explicit power-off chord, the firmware disables the deep-sleep GPIO wake
source, gives each live dynamic card a synchronous best-effort pre-sleep hook,
then calls `esp_deep_sleep_start()`. The Tamagotchi hook advances local time and
flushes its NVS record without waiting for Wi-Fi or SNTP; a failed flush is logged
and does not prevent sleep. Wake the device with the hardware reset control.

The shutdown path does not explicitly dim the backlight or stop Wi-Fi. Cards with
no persistent sleep work use the default no-op hook.

## NeoPixel

`NeoPixelController` uses FastLED for one WS2812B pixel. The data pin defaults to GPIO 33 when `NEOPIXEL_DATA_PIN` is not overridden. Board-defined NeoPixel power is enabled before FastLED initialization.

`neoPixelTask` calls `update()` every 5 ms; the controller limits visual updates to approximately every 16 ms. It generates a breathing RGB effect with a minimum 5% channel value.

The animation is currently independent of `SystemController` state. New status colors should define priority and avoid having several tasks write FastLED simultaneously.

## Storage and partitions

The flash partition table is documented in [OTA updates](ota.md). Review it before changing partitions or adding large assets.

## Reset and bootloader recovery

To enter the serial bootloader:

1. Hold down/BOOT(D0).
2. Press reset.
3. Release down/BOOT(D0).

PlatformIO recovery commands and their effects on persisted configuration are documented in [Build, test, and release](build-test-release.md#flash-and-recovery).
