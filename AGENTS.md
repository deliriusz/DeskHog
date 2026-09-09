# DeskHog agent reference

DeskHog is firmware for an Adafruit ESP32-S3 Reverse TFT Feather with a 240×135 display. It presents a configurable vertical stack of local cards, games, and PostHog insight visualizations. Wi-Fi, device settings, card configuration, and firmware updates are managed through an embedded web portal.

Use this file as the starting point for repository work, then open the focused guide for the subsystem you are changing.

## Stack

- C++ using Arduino/ESP32 and FreeRTOS, built with PlatformIO.
- LVGL 9.2.2 with an Adafruit ST7789 display driver.
- ESPAsyncWebServer, ArduinoJson, Bounce2, FastLED, and ESP32 Preferences/NVS.
- One PlatformIO environment: `adafruit_feather_esp32s3_reversetft`.
- PSRAM is required at startup; firmware uses a two-slot OTA partition table.

## Build and validation

The canonical build, upload, monitoring, test, and release instructions are in [Build, test, and release](docs/build-test-release.md). There is no enabled native test environment; do not report tests as run unless an environment is explicitly configured first.

## Choose the relevant guide

| When changing… | Read… |
|---|---|
| Startup, FreeRTOS tasks, core ownership | [Runtime and tasks](docs/runtime-and-tasks.md) |
| Cross-component messages | [Event queue](docs/event-queue.md) |
| TFT setup, LVGL, or background-to-UI updates | [Display and LVGL](docs/display-and-lvgl.md) |
| Card types, factories, configuration, or lifecycle | [Cards](docs/cards.md) |
| Porting an old hackathon or directly integrated feature | [Legacy card ports](docs/cards.md#porting-a-legacy-or-hackathon-card) |
| Buttons, input consumption, or card paging | [Input and navigation](docs/input-and-navigation.md) |
| Interactive or continuously updated cards | [Games](docs/games.md) |
| Station/AP behavior and on-device provisioning UI | [Wi-Fi and provisioning](docs/wifi-and-provisioning.md) |
| Embedded web routes and asynchronous portal actions | [Captive portal](docs/captive-portal.md) |
| NVS settings and overall readiness state | [Configuration and state](docs/configuration-and-state.md) |
| PostHog fetching, parsing, and visualization | [PostHog insights](docs/posthog-insights.md) |
| Firmware updates | [OTA updates](docs/ota.md) |
| Portal, font, or sprite generation | [Generated assets](docs/assets.md) |
| PlatformIO, flashing, tests, or CI releases | [Build, test, and release](docs/build-test-release.md) |
| Board pins, LED, memory, and sleep behavior | [Hardware and power](docs/hardware-and-power.md) |

## Project-wide rules and conventions

- Keep `src/main.cpp` focused on dependency wiring and task creation. Put behavior in focused `.h`/`.cpp` components.
- Treat the LVGL handler on core 1 as the UI owner. Marshal background results through the UI callback queue before changing LVGL objects; an event subscription alone does not change execution context.
- Use `EventQueue` for domain events between components. Verify producer and consumer context, queue-full behavior, and ownership of queued data.
- Add configurable card types in `src/config/CardConfig.h` and register their factories in `CardController::initializeCardTypes()`.
- Follow the existing C++/Arduino style: `#pragma once`, quoted project includes, PascalCase classes, and `camelCase` methods and variables. There is no configured formatter.
- Follow the source/generated ownership rules in [Generated assets](docs/assets.md), and review regenerated diffs after the next build.
- Never commit or log Wi-Fi passwords, PostHog personal API keys, or other secrets. Avoid adding secrets to URLs or query strings.
- Check the application-slot limit documented in [OTA updates](docs/ota.md) when adding assets or libraries.

## Review checklist

- Build the configured PlatformIO environment and report anything that could not be validated.
- For concurrent or UI changes, verify execution context, synchronization, queue-full behavior, lifetime, and stack/heap impact.
- For network and storage changes, review secret exposure and effects on persisted device state.
- For asset changes, inspect both source and regenerated output and check firmware size.

## Additional project context

- This is an ESP32-S3-based project that displays PostHog insights on a 240×135 TFT screen.
- Device details are available in [platformio.ini](platformio.ini).

## Hardware implementation

- Do not reinvent pin names or redefine them. Refer to the board definition's pins file whenever a pin is needed:
  `~/.platformio/packages/framework-arduinoespressif32/variants/adafruit_feather_esp32s3_reversetft/pins_arduino.h`

## Code generation

- Before generating code, carefully consider the existing project context, propose a plan for the code being written, and obtain clarification and approval to proceed when needed.
- Writing code should follow due consideration and planning rather than being an immediate response.
- Follow the existing C++/Arduino naming convention documented above: use `camelCase` for variables and methods. This supersedes the former Cursor rule's `snakeCase` wording.
- Keep concerns separated; for example, do not put network code into UI components.

## Troubleshooting

- When troubleshooting, writing new code is the final step, not the first. Do not randomly try new code.
- Consider up to five root causes of a failure, then narrow them down to the most likely one or two.
- Only after providing a detailed remediation plan should new code be written.
- Do not invent functions or properties on external libraries that do not actually exist.
- Switching away from the Arduino framework is not a viable strategy.
- If USB operations falter, this command can reset the whole USB subsystem:
  `sudo pkill -f "usb|serial|uart"; sudo pkill -f tty; sudo killall -STOP usbd; sleep 2; sudo killall -CONT usbd`
