# Event queue

`EventQueue` is the firmware's domain-message bus. It decouples producers such as configuration, Wi-Fi, and PostHog code from consumers such as `CardController`, `WiFiInterface`, and `InsightCard`.

It is separate from the UI callback queue described in [Display and LVGL](display-and-lvgl.md).

## Event payload

An `Event` contains:

- `type`: the `EventType` discriminator;
- `insightId`: an optional PostHog insight identifier;
- `parser`: an optional shared parsed insight;
- `jsonData`: optional raw insight JSON;
- `title`: optional card title text.

Prefer the smallest payload required by the consumer. Large raw JSON is currently transported for insight responses.

## Event catalog

| Event | Typical publisher | Consumer / effect |
|---|---|---|
| `WIFI_CREDENTIALS_FOUND` | `ConfigManager`, portal | `WiFiInterface` begins a stored-credential connection |
| `NEED_WIFI_CREDENTIALS` | `ConfigManager`, Wi-Fi state | `WiFiInterface` starts AP/captive-portal mode |
| `WIFI_CONNECTING` | `WiFiInterface` | `CardController` updates provisioning status |
| `WIFI_CONNECTED` | `WiFiInterface` | `CardController` shows connected status |
| `WIFI_CONNECTION_FAILED` | `WiFiInterface` | `CardController` shows failure status |
| `WIFI_AP_STARTED` | `WiFiInterface` | `CardController` shows the provisioning QR code |
| `CARD_CONFIG_CHANGED` | `ConfigManager::saveCardConfigs()` | `CardController` reloads and rebuilds configurable cards |
| `CARD_TITLE_UPDATED` | `InsightCard` | `CardController` persists the fetched insight title |
| `INSIGHT_DATA_RECEIVED` | `PostHogClient` | Matching `InsightCard` parses and displays the response |
| `INSIGHT_FORCE_REFRESH` | `InsightCard` center-button action | `PostHogClient` queues a blocking refresh request |
| `OTA_PROCESS_START` | Reserved | No active producer/consumer in the current OTA path |
| `OTA_PROCESS_END` | Reserved | No active producer/consumer in the current OTA path |

## Delivery semantics

- `EventQueue` is created with capacity 20 in `setup()`.
- Publishing uses `xQueueSend(..., 0)`: it never waits for space and returns `false` when full.
- The current callers generally do not retry failed publishes. Treat event loss as possible and log or recover where the event is critical.
- One `EventQueueTask` receives events and invokes every subscribed callback in registration order.
- A slow subscriber delays every later subscriber and event.
- Callbacks run while the callback-list mutex is held. Do not subscribe from inside a callback, block for long periods, or create callback cycles.
- There is no unsubscribe API. A subscriber capturing `this` must remain alive for the lifetime of the queue, or the event system must be extended before dynamically destroying it.

## Publishing

Use an existing overload when possible:

```cpp
eventQueue.publishEvent(EventType::WIFI_CONNECTED, "");
eventQueue.publishEvent(EventType::INSIGHT_DATA_RECEIVED, insightId, json);
eventQueue.publishEvent(Event::createTitleUpdateEvent(insightId, title));
```

Check the Boolean result when the event is required for correctness:

```cpp
if (!eventQueue.publishEvent(event)) {
    Serial.println("Failed to queue event");
}
```

## Subscribing

Filter immediately and keep the callback short:

```cpp
eventQueue.subscribe([this](const Event& event) {
    if (event.type != EventType::MY_EVENT) return;
    handleMyEvent(event);
});
```

An event subscription does **not** run on the UI task. If `handleMyEvent()` needs to change an LVGL object, copy the required data and enqueue a UI callback.

## Adding an event

1. Add a narrowly named value to `EventType`.
2. Decide which payload fields are required; avoid unrelated optional fields if a dedicated payload would be clearer.
3. Identify every publisher and subscriber.
4. Define behavior if publication fails or duplicate events arrive.
5. Keep network, storage, and rendering work out of the callback when it can be queued to their owning context.
6. Update the catalog above.

## Safety caveat

FreeRTOS queues copy item bytes. The current `Event` contains non-trivial C++ objects (`String` and `std::shared_ptr`), so its use with `xQueueCreate(..., sizeof(Event))` deserves special care and should not be copied as a generic safe pattern. A future hardening pass should queue pointers/owned envelopes or use a C++ queue that runs constructors and destructors correctly.
