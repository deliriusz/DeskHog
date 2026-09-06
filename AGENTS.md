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
