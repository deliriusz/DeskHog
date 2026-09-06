# Configuration and state

`ConfigManager` persists device configuration with ESP32 Preferences/NVS. `SystemController` maintains a separate in-memory readiness snapshot used by the provisioning UI and PostHog client.

## NVS namespaces

| Namespace | Current contents |
|---|---|
| `wifi_config` | SSID, password, credential-present flag, PostHog team ID, API key, and region |
| `cards` | JSON card array in `config_list` |
| `insights` | Opened for compatibility but not currently used by the active card configuration path |

`ConfigManager::begin()` opens all three namespaces read/write and derives the initial API configuration state.

## Keys and defaults

| Key | Type | Default / rule |
|---|---|---|
| `ssid` | String | Required when saving; maximum 32 characters |
| `password` | String | May be empty; maximum 64 characters |
| `has_creds` | Boolean | `false` |
| `team_id` | Integer | `NO_TEAM_ID` (`-1`) when absent; no range validation |
| `api_key` | String | Empty; required length 1–64 when setting |
| `region` | String | `us` when absent; otherwise stored without validation |
| `config_list` | JSON string | Empty card list when absent |

The card JSON document capacity is 2048 bytes. Each entry contains:

```json
{
  "type": "INSIGHT",
  "config": "abc123",
  "order": 0,
  "name": "Weekly active users"
}
```

## Persistence behavior

After a write, `commit()` closes and reopens all three Preferences instances. This means every write briefly affects the availability of all namespaces; callers should not assume concurrent access is safe.

Configuration events:

- Saving Wi-Fi credentials publishes `WIFI_CREDENTIALS_FOUND`.
- Clearing Wi-Fi credentials publishes `NEED_WIFI_CREDENTIALS`.
- `checkWiFiCredentialsAndPublish()` emits one of those events at boot.
- Saving card configuration publishes `CARD_CONFIG_CHANGED`.

Changing team ID, API key, or region recalculates `ApiState`; it does not publish a domain event.

`setApiKey()` enforces its documented length, but `setTeamId()` and `setRegion()` accept any value. The portal converts team ID with `toInt()` and ignores `setApiKey()` failure, so a successful response does not prove every value was stored.

## SystemController

`SystemController` is a singleton containing:

- `WifiState`: alias of `WiFiState`;
- `ApiState`: none, awaiting configuration, invalid, or configured;
- `AuthState`: none, awaiting login, or confirmed;
- `SystemState`: booting, ready, idle, or insights changed.

It accepts callbacks and invokes each callback immediately upon registration and after later changes. There is no callback removal per subscriber; `removeAllCallbacks()` clears the entire list.

`isSystemFullyReady()` requires:

1. Wi-Fi `CONNECTED`;
2. API `API_CONFIGURED`;
3. system state `SYS_READY`, `SYS_IDLE`, or `SYS_INSIGHTS_CHANGED`.

`PostHogClient` uses this predicate before processing requests.

`AuthState` and `SYS_INSIGHTS_CHANGED` currently have little or no active behavior. Do not build new logic around them without defining their lifecycle.

## Secrets

Wi-Fi passwords and PostHog personal API keys persist in NVS and survive reboots. Treat device configuration changes as real side effects.

- Never log passwords or full API keys.
- Do not commit real credentials in code, examples, captures, or generated portal output.
- The portal status response exposes only a masked API-key suffix.
- The PostHog client currently includes the API key in a query parameter; avoid introducing further exposure and prefer authenticated headers in future API work when supported.

## Threading caveat

Despite older class comments, `ConfigManager` has no mutex. Portal actions, event subscribers, and other tasks can reach it from different contexts. Keep operations short, avoid concurrent write paths, and add explicit synchronization before treating it as generally thread-safe.

Similarly, `SystemController` callback and state vectors are not mutex-protected. Register long-lived callbacks during initialization rather than mutating them concurrently.

## Changing the schema

When adding a setting:

1. Choose its namespace and stable key.
2. Define validation, maximum size, and absent-value behavior.
3. Decide whether existing devices require migration.
4. Add accessors that do not reveal secrets unnecessarily.
5. Define which state or event changes after a write.
6. Update the portal status/save flow if user-configurable.
7. Test both clean flash and upgrade from existing NVS.

See [Cards](cards.md) for card semantics and [Wi-Fi and provisioning](wifi-and-provisioning.md) for credential flow.
