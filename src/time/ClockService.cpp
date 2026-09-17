#include "time/ClockService.h"

#include <Arduino.h>

#include <cstdint>

#include <freertos/task.h>

#include "EventQueue.h"

static_assert(sizeof(time_t) <= sizeof(uint64_t),
              "ClockService requires time_t to fit in the persisted epoch type");

ClockService::ClockService(EventQueue& eventQueue)
    : _eventQueue(eventQueue),
      _stateMutex(nullptr),
      _state(ClockSyncState::NotStarted),
      _begun(false),
      _sntpConfigured(false),
      _initializationFailed(false) {}

void ClockService::begin() {
    if (_initializationFailed) {
        return;
    }

    if (_stateMutex == nullptr) {
        _stateMutex = xSemaphoreCreateMutex();
        if (_stateMutex == nullptr) {
            _initializationFailed = true;
            Serial.println("ClockService: failed to create state mutex");
            return;
        }
    }

    const time_t epoch = time(nullptr);
    const ClockSyncState initialState =
        isSaneEpoch(epoch) ? ClockSyncState::Synchronized : ClockSyncState::WaitingForNetwork;

    if (xSemaphoreTake(_stateMutex, portMAX_DELAY) != pdTRUE) {
        return;
    }

    if (_begun) {
        xSemaphoreGive(_stateMutex);
        return;
    }

    _begun = true;
    _state = initialState;
    _eventQueue.subscribe([this](const Event& event) { handleEvent(event); });

    xSemaphoreGive(_stateMutex);
}

void ClockService::requestSync() {
    time_t epoch = 0;
    if (tryGetEpoch(epoch)) {
        return;
    }

    if (_stateMutex == nullptr) {
        return;
    }

    bool configureSntp = false;
    if (xSemaphoreTake(_stateMutex, portMAX_DELAY) != pdTRUE) {
        return;
    }

    if (_begun && !_sntpConfigured) {
        // Reserve the sole configuration attempt before releasing the mutex so a
        // concurrent OTA request cannot issue a second configTime() call.
        _sntpConfigured = true;
        configureSntp = true;
    }

    xSemaphoreGive(_stateMutex);

    if (!configureSntp) {
        return;
    }

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("ClockService: requested network time synchronization");

    // A retained or just-synchronized epoch can become valid while configTime()
    // returns. Never overwrite that observation with SyncRequested.
    const ClockSnapshot current = snapshot();
    if (!current.valid) {
        setState(ClockSyncState::SyncRequested);
    }
}

ClockSnapshot ClockService::snapshot() {
    ClockSnapshot result;
    const time_t epoch = time(nullptr);
    const bool valid = isSaneEpoch(epoch);

    if (_stateMutex == nullptr) {
        return result;
    }

    if (xSemaphoreTake(_stateMutex, portMAX_DELAY) != pdTRUE) {
        return result;
    }

    if (valid) {
        _state = ClockSyncState::Synchronized;
    } else if (_state == ClockSyncState::Synchronized) {
        _state = _sntpConfigured ? ClockSyncState::SyncRequested
                                 : ClockSyncState::WaitingForNetwork;
    }

    result.state = _state;
    result.epoch = valid ? epoch : 0;
    result.valid = valid;

    xSemaphoreGive(_stateMutex);
    return result;
}

bool ClockService::tryGetEpoch(time_t& epoch) {
    const ClockSnapshot current = snapshot();
    if (!current.valid) {
        epoch = 0;
        return false;
    }

    epoch = current.epoch;
    return true;
}

bool ClockService::waitForValidTime(uint32_t timeoutMs) {
    requestSync();

    time_t epoch = 0;
    if (tryGetEpoch(epoch)) {
        return true;
    }

    const uint32_t startedAt = millis();
    while (static_cast<uint32_t>(millis() - startedAt) < timeoutMs) {
        vTaskDelay(pdMS_TO_TICKS(250));
        if (tryGetEpoch(epoch)) {
            return true;
        }
    }

    return tryGetEpoch(epoch);
}

bool ClockService::isSaneEpoch(time_t epoch) const {
    if (epoch < static_cast<time_t>(0) || epoch < minimumValidEpoch) {
        return false;
    }

    const uint64_t converted = static_cast<uint64_t>(epoch);
    return static_cast<time_t>(converted) == epoch;
}

void ClockService::handleEvent(const Event& event) {
    if (event.type != EventType::WIFI_CONNECTED) {
        return;
    }

    requestSync();
}

void ClockService::setState(ClockSyncState state) {
    if (_stateMutex == nullptr) {
        return;
    }

    if (xSemaphoreTake(_stateMutex, portMAX_DELAY) == pdTRUE) {
        // A concurrent snapshot may have observed a valid epoch after the
        // caller's invalid sample. Preserve that newer Synchronized state.
        if (_state != ClockSyncState::Synchronized || state == ClockSyncState::Synchronized) {
            _state = state;
        }
        xSemaphoreGive(_stateMutex);
    }
}
