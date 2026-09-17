#pragma once

#include <cstdint>
#include <ctime>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class EventQueue;
struct Event;

enum class ClockSyncState : uint8_t {
    NotStarted,
    WaitingForNetwork,
    SyncRequested,
    Synchronized
};

struct ClockSnapshot {
    ClockSyncState state = ClockSyncState::NotStarted;
    time_t epoch = 0;
    bool valid = false;
};

// Owns the firmware's single SNTP configuration path. It only reports whether the
// system clock is trustworthy; it does not retain a separate clock value.
class ClockService {
public:
    static constexpr time_t minimumValidEpoch = 1704067200; // 2024-01-01 UTC

    explicit ClockService(EventQueue& eventQueue);

    void begin();
    void requestSync();
    ClockSnapshot snapshot();
    bool tryGetEpoch(time_t& epoch);
    bool waitForValidTime(uint32_t timeoutMs);

private:
    EventQueue& _eventQueue;
    SemaphoreHandle_t _stateMutex;
    ClockSyncState _state;
    bool _begun;
    bool _sntpConfigured;
    bool _initializationFailed;

    bool isSaneEpoch(time_t epoch) const;
    void handleEvent(const Event& event);
    void setState(ClockSyncState state);
};
