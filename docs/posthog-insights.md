# PostHog insights

The insight subsystem fetches PostHog API responses on a background task, publishes raw JSON through `EventQueue`, parses the response, selects a renderer, and updates LVGL through the UI callback queue.

## End-to-end flow

1. An `INSIGHT` card factory creates `InsightCard` with an insight short ID.
2. The factory calls `PostHogClient::requestInsightData(id)`.
3. `insightTask` calls `PostHogClient::process()` every 100 ms.
4. When the system is ready, the client dequeues one request and calls the PostHog API.
5. A successful response is published as `INSIGHT_DATA_RECEIVED` with the ID and raw JSON.
6. Every `InsightCard` receives the event; only the matching ID parses it.
7. `InsightParser` detects the insight type and exposes normalized values.
8. `InsightCard` publishes `CARD_TITLE_UPDATED` when the fetched title changes.
9. The card dispatches renderer creation/update to the UI queue.
10. The renderer updates numeric, line-chart, or funnel LVGL objects.

## Readiness

The client processes requests only when:

- `SystemController::isSystemFullyReady()` is true;
- team ID is present;
- API key is non-empty;
- and, when fetching, Arduino Wi-Fi status is connected.

Requests can be queued before readiness and remain pending.

## Request behavior

The base URL is selected from the stored region:

```text
https://<region>.posthog.com/api/projects/<team-id>/insights/
```

Normal fetches first request `refresh=force_cache`. If the response text contains a null or empty result, the client makes a second request with `refresh=blocking`. A center-button refresh goes directly to blocking mode.

Failures are moved to the back of the FIFO queue with an incremented retry count and a one-second delay. The request is dropped after the configured retry limit.

Known insight IDs are retained for refresh. Every 30 minutes, `checkRefreshes()` selects one known insight and fetches it; the interval is not a promise that every card refreshes every 30 minutes.

## Force refresh

When center is pressed on an insight card:

1. The card publishes `INSIGHT_FORCE_REFRESH` with its ID.
2. It queues a title change to `Refreshing...`.
3. `PostHogClient` subscribes to the event and enqueues a forced request.
4. The normal response flow restores the fetched title and data.

Up/down are not consumed by `InsightCard`, so they continue to navigate.

## Parsing

`InsightParser` allocates a 64 KB `DynamicJsonDocument`, parses the response, locates the insight result object, and detects:

- numeric/bold-number cards;
- line graphs;
- area charts;
- funnels, including breakdowns and conversion metadata;
- unsupported structures.

Names and labels use bounded character buffers. Line values use a caller-allocated array; because `getSeriesYValues()` has no capacity argument, allocate exactly `getSeriesPointCount()` elements.

Parsing currently happens inside the `InsightCard` event callback. Because `EventQueueTask` is unpinned, parsing is not guaranteed to stay on core 0 even though HTTP fetching does.

## Renderer lifecycle

`InsightCard` maintains one `InsightRendererBase` implementation. When the detected type changes or the renderer's LVGL objects become invalid, it:

1. clears the old renderer;
2. cleans the content container;
3. creates the new renderer's elements;
4. invalidates and refreshes the layout;
5. supplies parsed data to `updateDisplay()`.

Current renderers:

| Parser type | Renderer | Behavior |
|---|---|---|
| `NUMERIC_CARD` | `NumericCardRenderer` | Formats a single value with optional prefix/suffix and K/M scaling |
| `LINE_GRAPH` | `LineGraphRenderer` | Creates an LVGL line chart, samples data when needed, and scales the Y range |
| `FUNNEL` | `FunnelRenderer` | Draws up to the renderer's supported steps and breakdown segments |
| `AREA_CHART` | Numeric fallback | Detected by the parser but has no dedicated renderer |
| Unsupported | Numeric fallback | Logs unsupported type and uses the numeric renderer |

Renderer `updateDisplay()` implementations dispatch their own UI callbacks. Because `InsightCard` already invokes them from a queued UI callback, an update can require an additional UI-queue cycle.

## Adding an insight visualization

1. Confirm the actual PostHog JSON variants with fixtures that contain no credentials.
2. Extend `InsightParser::InsightType` and structural detection.
3. Add bounded parser accessors; keep raw JSON knowledge out of the renderer.
4. Implement `InsightRendererBase`: `createElements`, `updateDisplay`, `clearElements`, and `areElementsValid`.
5. Add the renderer selection case in `InsightCard`.
6. Test first render, repeated updates, type changes, empty results, oversized series, and card removal.
7. Measure JSON, heap/PSRAM, UI queue, and firmware-size impact.

## Security and reliability

- `PostHogClient` currently calls `WiFiClientSecure::setInsecure()`. Do not use that as the model for new network clients; validate certificates where feasible.
- The personal API key is currently placed in the query string. Do not log complete request URLs.
- Region and team-ID validation limitations are documented in [Configuration and state](configuration-and-state.md); do not construct new API hosts from unvalidated input.
- `request_queue` and `requested_insights` have no explicit mutex even though requests can originate outside `insightTask`. Changes to producers should include a synchronization review.
- `EventQueue` transports raw JSON in an `Event` containing non-trivial C++ objects; see its [safety caveat](event-queue.md#safety-caveat).
- Insight-card subscriptions cannot currently be removed, while configured cards can be destroyed and recreated. Lifetime hardening is required before relying on long-running dynamic churn.
