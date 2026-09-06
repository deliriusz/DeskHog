# Wi-Fi and provisioning

`WiFiInterface` manages station connections, fallback access-point mode, DNS interception, network scans, and Wi-Fi state events. `ProvisioningCard` presents either the AP QR code or station/API status on the device.

## State machine

| State | Meaning | Main behavior |
|---|---|---|
| `DISCONNECTED` | No active station connection | May request credentials or wait for a new attempt |
| `CONNECTING` | `WiFi.begin()` has been called | `process()` enforces the configured timeout |
| `CONNECTED` | Arduino emitted station-connected | IP, SSID, and RSSI become available as the connection completes |
| `AP_MODE` | DeskHog provisioning AP is active | DNS sends all hostnames to `192.168.4.1` |

State changes also notify the legacy `WiFiInterface::onStateChange()` callback and `SystemController`.

## Boot flow

At startup, `ConfigManager::checkWiFiCredentialsAndPublish()` publishes one of:

- `WIFI_CREDENTIALS_FOUND`: load the saved SSID/password and call `connectToStoredNetwork(30000)`;
- `NEED_WIFI_CREDENTIALS`: start the DeskHog access point.

Connection attempts are non-blocking. `wifiTask` calls `WiFiInterface::process()` every 10 ms. If `CONNECTING` exceeds the timeout, it disconnects, publishes `WIFI_CONNECTION_FAILED`, and starts AP mode.

Arduino Wi-Fi events update the state. `ARDUINO_EVENT_WIFI_STA_CONNECTED` marks the interface connected before an IP address exists, and `ARDUINO_EVENT_WIFI_STA_GOT_IP` updates IP-related UI. The AP-transition defect is described under current caveats.

## Access-point mode

`startAccessPoint()`:

1. Switches to `WIFI_AP` mode.
2. Builds an SSID beginning with `DeskHog_` from part of the MAC address.
3. Configures `192.168.4.1/24`.
4. Starts an open AP; `_apPassword` is currently empty.
5. Starts a wildcard DNS server on port 53.
6. publishes `WIFI_AP_STARTED` and shows the QR screen.

`wifiTask` services the DNS server while state is `AP_MODE`. `stopAccessPoint()` stops/deletes DNS, disconnects the soft AP, and returns the radio to station mode.

## Network scans

`WiFiInterface::scanNetworks()` performs a blocking scan including hidden networks. Results retain SSID, RSSI, and encryption type. `CaptivePortal` converts them to JSON and caches them with a timestamp.

Portal scan scheduling, including its startup behavior, is documented in [Captive portal](captive-portal.md). Avoid adding blocking scans to an HTTP callback or the LVGL task.

## Wi-Fi events

`WiFiInterface::updateState()` publishes:

- `WIFI_CONNECTING`;
- `WIFI_CONNECTED`;
- `WIFI_AP_STARTED`;
- `NEED_WIFI_CREDENTIALS` when disconnected with no stored credentials.

Timeout failure is published separately as `WIFI_CONNECTION_FAILED`. Consumers must remember that event subscribers run on `EventQueueTask`, not the UI task.

## Provisioning card

The permanent `ProvisioningCard` contains two child screens:

- **QR screen:** firmware version, Wi-Fi QR code, and AP SSID;
- **status screen:** Wi-Fi name/status, station IP, signal percentage, firmware version, and PostHog API configuration state.

The QR payload follows the standard Wi-Fi format and escapes backslashes, semicolons, commas, quotes, and apostrophes. Because the AP is open, its payload uses `T:nopass`.

The card listens to `SystemController` for Wi-Fi/API state and receives more specific connection, IP, and signal updates from `WiFiInterface` and `CardController`.

## Saving credentials

The portal's `SAVE_WIFI` action calls `ConfigManager::saveWiFiCredentials()`. That method persists the SSID/password and publishes `WIFI_CREDENTIALS_FOUND`; the portal currently publishes the same event again after a successful save. Consumers should tolerate duplicate connection requests.

Wi-Fi credentials remain in NVS until explicitly cleared or flash is erased. Never print the password or include it in status responses.

## Signal strength

When connected, RSSI is mapped to a percentage:

- at or below -100 dBm: 0%;
- at or above -50 dBm: 100%;
- between them: `2 * (RSSI + 100)`.

The provisioning UI is refreshed about every five seconds while connected.

## Current caveats

- Some `WiFiInterface` callbacks directly invoke provisioning-card methods. New UI changes should be dispatched through the UI queue described in [Display and LVGL](display-and-lvgl.md).
- Every credentials-found event sets `_attemptingNewConnectionAfterPortal`, including at boot. The next `process()` call stops the AP and clears the flag without verifying connectivity, usually before got-IP can handle it.

See [Captive portal](captive-portal.md) for HTTP behavior and [Configuration and state](configuration-and-state.md) for NVS storage.
