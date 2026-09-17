#pragma once

#include <cstdint>

#include <Preferences.h>

#include "tamagotchi/TamagotchiTypes.h"

enum class TamagotchiLoadOrigin : uint8_t {
    StoredV1 = 0,
    Missing = 1,
    InvalidRecord = 2,
    UnsupportedSchema = 3,
    StorageUnavailable = 4
};

struct TamagotchiLoadResult {
    TamagotchiState state{};
    TamagotchiLoadOrigin origin = TamagotchiLoadOrigin::StorageUnavailable;
    bool persisted = false;
};

class TamagotchiStateStore {
public:
    TamagotchiStateStore() = default;
    ~TamagotchiStateStore();

    TamagotchiStateStore(const TamagotchiStateStore&) = delete;
    TamagotchiStateStore& operator=(const TamagotchiStateStore&) = delete;
    TamagotchiStateStore(TamagotchiStateStore&&) = delete;
    TamagotchiStateStore& operator=(TamagotchiStateStore&&) = delete;

    bool begin();
    void end();
    bool isReady() const;
    TamagotchiLoadResult load();
    bool save(const TamagotchiState& state);

private:
    Preferences preferences;
    bool ready = false;
};
