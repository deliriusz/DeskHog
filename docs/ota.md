# OTA updates

DeskHog checks GitHub Releases for `PostHog/DeskHog`, downloads an asset named `firmware.bin`, writes it with the ESP32 Arduino `Update` library, selects the new boot partition, and restarts.

OTA operations are exposed through the [captive portal](captive-portal.md) and run in dedicated core-0 tasks.

## Partition layout

`partitions.csv` defines:

| Partition | Offset | Size |
|---|---:|---:|
| NVS | `0x9000` | `0x6000` |
| OTA metadata | `0xf000` | `0x2000` |
| PHY init | `0x11000` | `0x1000` |
| `ota_0` application | `0x20000` | `0x1F0000` |
| `ota_1` application | `0x210000` | `0x1F0000` |

Every release build must fit within one `0x1F0000`-byte application slot. There is no filesystem partition in the current table.

## Checking for an update

`OtaManager::checkForUpdate()` refuses to start while a check/download/write is active, sets status to `CHECKING_VERSION`, and creates `otaCheckTask` on core 0.

The worker:

1. Asks the boot-lifetime `ClockService` to ensure a valid UTC epoch, waiting no more than approximately ten seconds. `ClockService` is the sole SNTP/configuration owner and normally begins synchronization asynchronously after `WIFI_CONNECTED`.
2. Requests `https://api.github.com/repos/PostHog/DeskHog/releases` using the embedded root CA.
3. Parses the first release in the returned array.
4. Reads `tag_name`, release notes, and the asset named `firmware.bin`.
5. Compares the tag with `CURRENT_FIRMWARE_VERSION`.
6. Stores `UpdateInfo` and a status/message for portal polling.

The version comparison currently removes an optional `v` prefix and performs lexicographical string comparison, not numeric semantic-version comparison. Release tags should remain consistently formatted until this is hardened.

## Installing an update

`beginUpdate()` requires a non-empty download URL and no download/write already in progress. It copies the URL into task-owned memory and starts `otaUpdateTask` on core 0.

The update task:

1. Verifies Wi-Fi connectivity.
2. Opens the HTTPS download with strict redirect following.
3. Reads the response content length.
4. Calls `Update.begin(totalSize, U_FLASH)` so the Arduino update layer selects the inactive application partition.
5. Streams data in 1460-byte chunks through `Update.write()`.
6. Updates status/progress while feeding the task watchdog.
7. Verifies the complete length and calls `Update.end(true)` to select the new boot partition.
8. Reports success, waits one second, and calls `ESP.restart()`.

Most failures abort and record an error. A zero or missing content length is different: the task records `ERROR_HTTP_DOWNLOAD` but continues into `Update.begin()` with the invalid size. Positive-length enforcement is currently broken.

## Status model

`UpdateStatus` includes:

- `IDLE`, `CHECKING_VERSION`, `DOWNLOADING`, `WRITING`, and `SUCCESS`;
- Wi-Fi, HTTP check/download, JSON, begin/write/end, missing asset, space, mutex, and internal error states;
- progress from 0–100 and a human-readable message.

`UpdateInfo` contains current/available versions, availability, download URL, release notes, and error text.

Public getters and most writes use `_dataMutex`. Busy getters return a temporary busy/error result instead of waiting. Protection is incomplete because `_updateTaskRunner` reads `_currentStatus.progress` without the mutex.

## Release requirements

The stable GitHub release asset must be named `firmware.bin`; changing that contract also requires changing `_firmwareAssetName` in `OtaManager`. Artifact production and its current serial-flashing warnings are documented in [Build, test, and release](build-test-release.md).

## Security and recovery

- Update checks and downloads use an embedded CA certificate. Certificate-chain changes or expiry can break OTA and require a serial update.
- Firmware is transported over validated HTTPS, but application-level signature verification is not implemented.
- Keep a serial flashing path available for certificate, partition, or boot failures.
- Never start OTA without stable power and Wi-Fi.
- The release version must be tested as a clean serial flash before relying on OTA distribution.
- Do not use the current generated flash command or `multi_flash.py -f` recovery path until their application offsets are corrected; see the build guide.

## Current caveats

- Version comparison is lexicographical.
- `OTA_PROCESS_START` and `OTA_PROCESS_END` exist in `EventType` but are not used by `OtaManager`.
- Some late error paths call `_setUpdateStatus()` while holding the same non-recursive `_dataMutex`, risking deadlock.
- OTA logging is verbose and includes the download URL. Do not add credentials or signed private URLs to that path.

See [Build, test, and release](build-test-release.md) for artifact production and [Hardware and power](hardware-and-power.md#reset-and-bootloader-recovery) for the physical bootloader sequence.
