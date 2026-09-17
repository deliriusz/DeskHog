# Captive portal

`CaptivePortal` serves the embedded setup application on port 80. It manages Wi-Fi scanning and credentials, PostHog device settings, card configuration, OTA actions, and captive-portal detection URLs.

The HTTP server starts during every boot, not only while the device is in AP mode. In provisioning mode, wildcard DNS directs clients to `192.168.4.1`.

## Portal assets

Edit the portal sources, not the generated header. The canonical input, generation, offline-operation, and size guidance is in [Generated assets](assets.md).

`CaptivePortal::begin()` performs one synchronous Wi-Fi scan before registering routes and starting the server. Subsequent scans requested through the current portal UI use the action worker described below.

## Asynchronous action model

The current `POST /api/actions/...` routes use a portal-specific `std::vector<QueuedAction>` with capacity 5:

1. A `POST /api/actions/...` route extracts form parameters.
2. `requestAction()` appends the action and returns HTTP 202.
3. If full, it returns HTTP 429 with `status: "queue_full"`.
4. `portalTask` calls `processAsyncOperations()` every 100 ms.
5. The worker removes the first action, executes it, and records completion status.
6. The browser polls `GET /api/status` for results.

This action queue is independent of both `EventQueue` and the LVGL UI callback queue.

Not every state-changing route uses it. `POST /api/cards/configured` performs one bounded, synchronous NVS replacement after validating its complete request body. New slow operations should use an owning worker rather than extending that synchronous pattern.

## Primary routes

| Method and path | Purpose |
|---|---|
| `GET /` | Embedded portal application |
| `GET /api/status` | Portal action, Wi-Fi, device configuration, and OTA status |
| `POST /api/actions/start-wifi-scan` | Queue a blocking Wi-Fi scan |
| `POST /api/actions/save-wifi` | Queue SSID/password persistence and connection |
| `POST /api/actions/save-device-config` | Queue team ID, API key, and region persistence |
| `POST /api/actions/check-ota-update` | Queue an OTA release check |
| `POST /api/actions/start-ota-update` | Queue installation of the last discovered update |
| `GET /api/cards/definitions` | Card catalog from `CardController` |
| `GET /api/cards/configured` | Persisted ordered card list |
| `POST /api/cards/configured` | Replace the persisted card list |

The current portal JavaScript uses the `/api/actions/...` routes and `/api/status` polling.

## Status response

`GET /api/status` contains four top-level objects:

- `portal`: pending action, last completed action, success/error, and message;
- `wifi`: scan status/results, last scan time, connected SSID, IP, and connection flag;
- `device_config`: team ID, masked API key display/presence, and region;
- `ota`: state code/message, progress, current/available versions, release notes, and error.

Responses disable caching and currently allow cross-origin access with `Access-Control-Allow-Origin: *`.

The `portal` fields are unreliable with several queued actions: a new request overwrites the single pending value, and completing the oldest action resets it to `NONE` even if work remains. Treat them as best-effort UI status.

## Card API

`GET /api/cards/definitions` returns:

```json
{
  "id": "INSIGHT",
  "name": "PostHog insight",
  "allowMultiple": true,
  "needsConfigInput": true,
  "configInputLabel": "Insight ID",
  "description": "Insight cards let you keep an eye on PostHog data"
}
```

`GET` and `POST /api/cards/configured` exchange arrays containing `type`, `config`, `order`, and `name`. A successful save publishes `CARD_CONFIG_CHANGED`, causing `CardController` to rebuild configurable cards.

The POST body must be JSON and no larger than 2048 bytes. It is assembled by contiguous offsets before parsing, so chunked requests receive the same validation as single-chunk requests. The root must be an array of at most 16 objects. Every object requires an exact, registered `type` string and an integer `order`; orders must be the contiguous permutation `0..N-1`. Optional `config` and `name` default to empty strings, must be strings of at most 64 bytes when supplied, and `config` must be nonempty/non-whitespace only for definitions that request it. No-config cards, including `TAMAGOTCHI`, require an empty config value.

Singleton policy comes from each registered definition's `allowMultiple` value. A second singleton entry rejects the entire request before NVS is touched; repeatable cards remain allowed. Unknown type strings are never treated as insights.

Successful writes return HTTP 200 with `success`, `message`, and `count`. Validation errors return HTTP 400 with a stable `error.code` and, where applicable, array `index` and field. Oversized payloads return 413, explicit non-JSON media types return 415, and body/storage failures return 500. All responses are JSON and include the permissive CORS header.

## Captive-portal detection

The server handles common probes including:

- `/generate_204`;
- `/hotspot-detect.html`;
- `/connecttest.txt`;
- `/ncsi.txt`;
- `/fwlink`;
- `/mobile/status.php`.

While not connected upstream, probes and unknown routes redirect to `/`. When connected, known probes receive the success response expected by the requesting platform.

## Legacy routes

Compatibility routes remain for `/scan-networks`, `/get-device-config`, `/check-update`, `/start-update`, and `/update-status`; the current UI uses the action/status API. Verify registration before relying on other legacy handlers because some old save methods have no corresponding POST route.

## Adding a portal operation

1. Add a `PortalAction` value and string conversion.
2. Register a short request handler that calls `requestAction()`.
3. Copy all required request parameters into `QueuedAction`; request objects do not outlive the HTTP callback.
4. Implement the operation in `processAsyncOperations()`.
5. Expose stable status/results from `/api/status`.
6. Update `html/portal.js` to queue and poll.
7. Regenerate `include/html_portal.h` through the normal build.

## Security and concurrency

- The portal can write real NVS configuration and start firmware updates. Do not expose secrets in responses or logs.
- API keys are returned only as a masked suffix; unchanged placeholder values are not persisted as new keys.
- Device-setting validation and the route's misleading save-success behavior are documented in [Configuration and state](configuration-and-state.md).
- There is currently no portal authentication and CORS is permissive. Assume any client with network access to the device can call its routes.
- The action vector and status fields are accessed by HTTP and portal-task contexts without an explicit mutex. Avoid widening concurrent access without first adding synchronization.
- Card-configuration requests are capped at 2048 bytes; errors during body allocation, chunk ordering, bounds checks, or parsing do not write NVS or publish a configuration event.
