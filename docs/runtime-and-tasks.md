# Runtime and tasks

This page describes the current startup order and FreeRTOS execution contexts. Treat task ownership as an architectural contract: a function being thread-safe does not make LVGL itself safe to call from an arbitrary task.

## Startup sequence

Arduino calls `setup()` once. The firmware then:

1. Starts serial logging and requires PSRAM initialization to succeed.
2. Enables dynamic frequency scaling and automatic light sleep.
3. Logs the partition table.
4. Initializes `SystemController` and shared fonts/styles.
5. Creates and starts the `EventQueue`.
6. Creates the boot-lifetime `ClockService`, which subscribes to `WIFI_CONNECTED` before Wi-Fi starts.
7. Creates `NeoPixelController` and `ConfigManager`.
8. Creates `PostHogClient`.
9. Creates `DisplayInterface`, initializes the ST7789, and starts LVGL.
10. Creates `WiFiInterface` and registers Wi-Fi event handling.
11. Configures the three physical buttons and GPIO light-sleep wake sources.
12. Creates `CardController`, its UI callback queue, the card stack, and configured cards.
13. Creates `OtaManager` and `CaptivePortal`, performs the portal startup described in [Captive portal](captive-portal.md), and starts the HTTP server.
14. Starts the long-running tasks listed below.
15. Publishes the initial Wi-Fi credential event and marks the system ready.

The Arduino `loop()` immediately deletes its own task; ongoing work belongs to FreeRTOS tasks.

## Task map

| Task | Core | Priority | Stack | Cadence / trigger | Responsibility |
|---|---:|---:|---:|---|---|
| `wifiTask` | 0 | 1 | 4096 | 10 ms | DNS processing, connect timeouts, and periodic signal updates |
| `insightTask` | 0 | 1 | 8192 | 100 ms | PostHog request queue and refresh checks |
| `neoPixelTask` | 0 | 1 | 2048 | 5 ms loop; controller limits itself to ~16 ms | Breathing NeoPixel animation |
| `portalTask` | 1 | 1 | 8192 | 100 ms | Executes queued portal actions such as scans and settings writes |
| `lv_tick_task` | 1 | 1 | 2048 | 10 ms | Advances LVGL time with `lv_tick_inc()` |
| `lvglTask` | 1 | 2 | 8192 | 5 ms | Runs LVGL timers, drains the UI callback queue, updates the active card, and polls buttons every 50 ms |
| `EventQueueTask` | Unpinned | idle + 1 | 4096 | Event-driven, 100 ms receive timeout | Delivers domain events to all subscribers |
| `otaCheckTask` | 0 | 1 | 8192 | On demand | Bounded wait for shared network time, then GitHub release check |
| `otaUpdateTask` | 0 | 2 | 12288 | On demand | Firmware download, inactive-partition write, and restart |

ESPAsyncWebServer also invokes HTTP callbacks in its networking context. Route handlers should respond quickly and enqueue slow work for `portalTask`.

## Ownership rules

- Core affinity alone does not establish UI safety. Only work executed through the LVGL handler/UI callback path should mutate the LVGL object tree.
- `EventQueueTask` is unpinned. Event subscribers run synchronously on that task while the callback-list mutex is held.
- `portalTask` currently runs on core 1, but it is not the LVGL task and must not directly render UI.
- The active card's `update()` method runs from `CardController::processUIQueue()` in `lvglTask`.
- Long network operations belong on the insight, portal, or OTA workers, not in input handling or rendering.
- `ClockService` starts SNTP asynchronously in its short `WIFI_CONNECTED` subscriber. It owns no task and never accesses LVGL, NVS, cards, or OTA state; OTA and future cards only sample its epoch/status API.

## Main data flows

### Configuration change

`CaptivePortal` → `ConfigManager::saveCardConfigs()` → `CARD_CONFIG_CHANGED` → `CardController` → UI callback queue → rebuild card stack.

### PostHog insight

Card factory → `PostHogClient::requestInsightData()` → `insightTask` → `INSIGHT_DATA_RECEIVED` → `InsightCard` parser → UI callback queue → renderer.

### Wi-Fi provisioning

Stored/portal credentials → `WIFI_CREDENTIALS_FOUND` → `WiFiInterface` → Arduino Wi-Fi events → Wi-Fi state event → provisioning UI update.

## Adding background work

Before adding a task, prefer extending an existing component's `process()` method or publishing an event. If a new task is necessary, document:

- its core, priority, stack size, cadence, and blocking behavior;
- who owns and destroys its state;
- how it communicates results;
- how queue saturation and allocation failure are handled;
- whether its callbacks can reach LVGL, NVS, Wi-Fi, or other shared resources.

## Current caveats

- Older documentation placed the portal worker on core 0. `main.cpp` currently pins it to core 1.
- Insight HTTP fetching runs on core 0, but JSON parsing occurs in the `InsightCard` event subscriber and therefore on the unpinned event task.
- Initial card/LVGL creation happens during `setup()`. Subsequent background-driven changes should use the UI callback queue.
