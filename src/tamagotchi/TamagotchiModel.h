#pragma once

#include <cstdint>

#include "tamagotchi/TamagotchiTypes.h"

// Transient result for a care action. It is intentionally not persisted.
struct TamagotchiActionOutcome {
    TamagotchiActionResult result = TamagotchiActionResult::InvalidAction;
    TamagotchiModelChange changes = TamagotchiModelChange::None;
};

// Owns one validated state value and its accumulated persistence-dirty mask.
// The model is single-owner and single-threaded; callers can only inspect state.
class TamagotchiModel {
public:
    explicit TamagotchiModel(const TamagotchiState& initialState = TamagotchiState{});

    const TamagotchiState& getState() const;

    TamagotchiModelChange hatch();
    TamagotchiActionOutcome performAction(TamagotchiAction action);
    TamagotchiModelChange advanceBy(uint64_t elapsedSeconds);

    TamagotchiModelChange setLastUpdatedEpoch(uint64_t epochSeconds);

    bool isDirty() const;
    TamagotchiModelChange getDirtyChanges() const;
    void markPersisted();

private:
    TamagotchiModelChange processMinuteStep();
    TamagotchiModelChange fastForwardStableState(uint64_t minuteSteps);
    void recordChanges(TamagotchiModelChange changes);

    static uint8_t applyNeedDelta(uint8_t value, int16_t delta);
    static bool crossedBoundary(uint64_t oldAge, uint64_t newAge, uint64_t interval);
    static uint64_t saturatingAdd(uint64_t value, uint64_t increment);
    static uint64_t saturatingAddMinutes(uint64_t age, uint64_t minuteSteps);
    static bool canFastForward(const TamagotchiState& state);

    TamagotchiState _state;
    TamagotchiModelChange _dirtyChanges = TamagotchiModelChange::None;
};
