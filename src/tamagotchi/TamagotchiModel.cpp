#include "tamagotchi/TamagotchiModel.h"

#include <limits>

namespace {

TamagotchiModelChange changesBetween(const TamagotchiState& before,
                                     const TamagotchiState& after) {
    TamagotchiModelChange changes = TamagotchiModelChange::None;

    if (before.hunger != after.hunger || before.happiness != after.happiness ||
        before.health != after.health || before.energy != after.energy ||
        before.hygiene != after.hygiene) {
        changes |= TamagotchiModelChange::Needs;
    }

    if (before.stage != after.stage) {
        changes |= TamagotchiModelChange::Stage;
    }

    if (before.sick != after.sick) {
        changes |= TamagotchiModelChange::SickState;
    }

    if (before.mess != after.mess) {
        changes |= TamagotchiModelChange::MessState;
    }

    if (before.ageSeconds != after.ageSeconds ||
        before.simulationRemainderSeconds != after.simulationRemainderSeconds ||
        before.nextMessAtAgeSeconds != after.nextMessAtAgeSeconds ||
        before.lastUpdatedEpoch != after.lastUpdatedEpoch) {
        changes |= TamagotchiModelChange::SimulationTime;
    }

    return changes;
}

} // namespace

TamagotchiModel::TamagotchiModel(const TamagotchiState& initialState)
    : _state(initialState) {}

const TamagotchiState& TamagotchiModel::getState() const {
    return _state;
}

TamagotchiModelChange TamagotchiModel::hatch() {
    if (_state.stage != TamagotchiStage::Egg) {
        return TamagotchiModelChange::None;
    }

    const TamagotchiState before = _state;

    _state.stage = TamagotchiStage::Child;
    _state.hunger = TamagotchiConstants::STARTING_HUNGER;
    _state.happiness = TamagotchiConstants::STARTING_HAPPINESS;
    _state.health = TamagotchiConstants::STARTING_HEALTH;
    _state.energy = TamagotchiConstants::STARTING_ENERGY;
    _state.hygiene = TamagotchiConstants::STARTING_HYGIENE;
    _state.ageSeconds = 0;
    _state.simulationRemainderSeconds = 0;
    _state.sick = false;
    _state.mess = false;
    _state.nextMessAtAgeSeconds = TamagotchiConstants::MESS_INTERVAL_SECONDS;

    const TamagotchiModelChange changes = changesBetween(before, _state);
    recordChanges(changes);
    return changes;
}

TamagotchiActionOutcome TamagotchiModel::performAction(TamagotchiAction action) {
    TamagotchiActionOutcome outcome;

    if (_state.stage == TamagotchiStage::Egg) {
        outcome.result = TamagotchiActionResult::RefusedWhileEgg;
        return outcome;
    }

    const TamagotchiState before = _state;

    switch (action) {
    case TamagotchiAction::Feed:
        _state.hunger = applyNeedDelta(_state.hunger, TamagotchiConstants::FEED_HUNGER_DELTA);
        _state.happiness =
            applyNeedDelta(_state.happiness, TamagotchiConstants::FEED_HAPPINESS_DELTA);
        _state.health = applyNeedDelta(_state.health, TamagotchiConstants::FEED_HEALTH_DELTA);
        outcome.result = TamagotchiActionResult::Applied;
        break;

    case TamagotchiAction::Play:
        if (_state.energy < TamagotchiConstants::PLAY_MINIMUM_ENERGY) {
            outcome.result = TamagotchiActionResult::RefusedLowEnergy;
            return outcome;
        }
        _state.happiness =
            applyNeedDelta(_state.happiness, TamagotchiConstants::PLAY_HAPPINESS_DELTA);
        _state.energy = applyNeedDelta(_state.energy, TamagotchiConstants::PLAY_ENERGY_DELTA);
        outcome.result = TamagotchiActionResult::Applied;
        break;

    case TamagotchiAction::Clean:
        _state.hygiene = TamagotchiConstants::CLEAN_HYGIENE_VALUE;
        _state.health = applyNeedDelta(_state.health, TamagotchiConstants::CLEAN_HEALTH_DELTA);
        _state.mess = false;
        _state.nextMessAtAgeSeconds =
            saturatingAdd(_state.ageSeconds, TamagotchiConstants::MESS_INTERVAL_SECONDS);
        if (_state.nextMessAtAgeSeconds == _state.ageSeconds) {
            _state.mess = true;
        }
        outcome.result = TamagotchiActionResult::Applied;
        break;

    case TamagotchiAction::Rest:
        _state.energy = applyNeedDelta(_state.energy, TamagotchiConstants::REST_ENERGY_DELTA);
        _state.health = applyNeedDelta(_state.health, TamagotchiConstants::REST_HEALTH_DELTA);
        outcome.result = TamagotchiActionResult::Applied;
        break;

    case TamagotchiAction::Doctor:
        if (!_state.sick) {
            outcome.result = TamagotchiActionResult::RefusedNotSick;
            return outcome;
        }
        _state.sick = false;
        if (_state.health < TamagotchiConstants::DOCTOR_MINIMUM_HEALTH) {
            _state.health = TamagotchiConstants::DOCTOR_MINIMUM_HEALTH;
        }
        outcome.result = TamagotchiActionResult::Applied;
        break;

    case TamagotchiAction::Count:
    default:
        outcome.result = TamagotchiActionResult::InvalidAction;
        return outcome;
    }

    outcome.changes = changesBetween(before, _state);
    recordChanges(outcome.changes);
    return outcome;
}

TamagotchiModelChange TamagotchiModel::advanceBy(uint64_t elapsedSeconds) {
    if (_state.stage == TamagotchiStage::Egg) {
        return TamagotchiModelChange::None;
    }

    const TamagotchiState before = _state;
    const uint64_t stepSeconds = TamagotchiConstants::SIMULATION_STEP_SECONDS;
    uint64_t minuteSteps = elapsedSeconds / stepSeconds;
    const uint64_t tailSeconds = elapsedSeconds % stepSeconds;
    uint16_t remainder = static_cast<uint16_t>(_state.simulationRemainderSeconds) +
                         static_cast<uint16_t>(tailSeconds);

    if (remainder >= stepSeconds) {
        ++minuteSteps;
        remainder -= stepSeconds;
    }
    _state.simulationRemainderSeconds = static_cast<uint8_t>(remainder);

    while (minuteSteps != 0) {
        const uint64_t oldAge = _state.ageSeconds;
        processMinuteStep();
        --minuteSteps;

        if (canFastForward(_state)) {
            fastForwardStableState(minuteSteps);
            break;
        }

        if (_state.ageSeconds == oldAge) {
            break;
        }
    }

    const TamagotchiModelChange changes = changesBetween(before, _state);
    recordChanges(changes);
    return changes;
}

TamagotchiModelChange TamagotchiModel::setLastUpdatedEpoch(uint64_t epochSeconds) {
    const bool validEpoch = epochSeconds == TamagotchiConstants::NO_CLOCK_EPOCH ||
                            (epochSeconds >= TamagotchiConstants::MIN_VALID_EPOCH &&
                             epochSeconds <= TamagotchiConstants::MAX_VALID_EPOCH);
    if (!validEpoch || _state.lastUpdatedEpoch == epochSeconds) {
        return TamagotchiModelChange::None;
    }

    _state.lastUpdatedEpoch = epochSeconds;
    const TamagotchiModelChange changes = TamagotchiModelChange::SimulationTime;
    recordChanges(changes);
    return changes;
}

bool TamagotchiModel::isDirty() const {
    return _dirtyChanges != TamagotchiModelChange::None;
}

TamagotchiModelChange TamagotchiModel::getDirtyChanges() const {
    return _dirtyChanges;
}

void TamagotchiModel::markPersisted() {
    _dirtyChanges = TamagotchiModelChange::None;
}

TamagotchiModelChange TamagotchiModel::processMinuteStep() {
    const TamagotchiState before = _state;
    const uint64_t oldAge = _state.ageSeconds;
    const uint64_t newAge =
        saturatingAdd(oldAge, TamagotchiConstants::SIMULATION_STEP_SECONDS);
    if (newAge == oldAge) {
        return TamagotchiModelChange::None;
    }

    _state.ageSeconds = newAge;

    if (!_state.mess && _state.nextMessAtAgeSeconds != 0 &&
        newAge >= _state.nextMessAtAgeSeconds) {
        _state.mess = true;
    }

    if (crossedBoundary(oldAge, newAge, TamagotchiConstants::HUNGER_DECAY_INTERVAL_SECONDS)) {
        _state.hunger = applyNeedDelta(_state.hunger, -1);
    }
    if (crossedBoundary(oldAge, newAge,
                        TamagotchiConstants::HAPPINESS_DECAY_INTERVAL_SECONDS)) {
        _state.happiness = applyNeedDelta(_state.happiness, -1);
    }
    if (crossedBoundary(oldAge, newAge, TamagotchiConstants::ENERGY_DECAY_INTERVAL_SECONDS)) {
        _state.energy = applyNeedDelta(_state.energy, -1);
    }
    if (crossedBoundary(oldAge, newAge, TamagotchiConstants::HYGIENE_DECAY_INTERVAL_SECONDS)) {
        _state.hygiene = applyNeedDelta(_state.hygiene, -1);
    }
    if (_state.mess &&
        crossedBoundary(oldAge, newAge,
                        TamagotchiConstants::MESS_HYGIENE_PENALTY_INTERVAL_SECONDS)) {
        _state.hygiene = applyNeedDelta(_state.hygiene, -1);
    }

    const bool sickAtStepStart = before.sick;
    const bool hasSickOrNeglectedCondition =
        sickAtStepStart || _state.hunger <= TamagotchiConstants::SEVERE_NEED_THRESHOLD ||
        _state.hygiene <= TamagotchiConstants::SEVERE_NEED_THRESHOLD;
    if (hasSickOrNeglectedCondition &&
        crossedBoundary(oldAge, newAge,
                        TamagotchiConstants::SICK_OR_NEGLECT_HAPPINESS_PENALTY_INTERVAL_SECONDS)) {
        _state.happiness = applyNeedDelta(_state.happiness, -1);
    }
    if (hasSickOrNeglectedCondition &&
        crossedBoundary(oldAge, newAge, TamagotchiConstants::HEALTH_DECAY_INTERVAL_SECONDS)) {
        _state.health = applyNeedDelta(_state.health, -1);
    }

    if (_state.health <= TamagotchiConstants::SICKNESS_HEALTH_THRESHOLD ||
        _state.hygiene <= TamagotchiConstants::SICKNESS_HYGIENE_THRESHOLD) {
        _state.sick = true;
    }

    if (_state.stage == TamagotchiStage::Child &&
        _state.ageSeconds >= TamagotchiConstants::CHILD_TO_ADULT_AGE_SECONDS) {
        _state.stage = TamagotchiStage::Adult;
    }

    return changesBetween(before, _state);
}

TamagotchiModelChange TamagotchiModel::fastForwardStableState(uint64_t minuteSteps) {
    const TamagotchiState before = _state;
    const uint64_t oldAge = _state.ageSeconds;
    const uint64_t newAge = saturatingAddMinutes(oldAge, minuteSteps);

    _state.ageSeconds = newAge;
    if (!_state.mess && _state.nextMessAtAgeSeconds != 0 &&
        newAge >= _state.nextMessAtAgeSeconds) {
        _state.mess = true;
    }

    return changesBetween(before, _state);
}

void TamagotchiModel::recordChanges(TamagotchiModelChange changes) {
    _dirtyChanges |= changes;
}

uint8_t TamagotchiModel::applyNeedDelta(uint8_t value, int16_t delta) {
    const int32_t adjusted = static_cast<int32_t>(value) + static_cast<int32_t>(delta);
    if (adjusted <= static_cast<int32_t>(TamagotchiConstants::MIN_NEED)) {
        return TamagotchiConstants::MIN_NEED;
    }
    if (adjusted >= static_cast<int32_t>(TamagotchiConstants::MAX_NEED)) {
        return TamagotchiConstants::MAX_NEED;
    }
    return static_cast<uint8_t>(adjusted);
}

bool TamagotchiModel::crossedBoundary(uint64_t oldAge, uint64_t newAge,
                                      uint64_t interval) {
    return interval != 0 && newAge / interval > oldAge / interval;
}

uint64_t TamagotchiModel::saturatingAdd(uint64_t value, uint64_t increment) {
    const uint64_t maximum = std::numeric_limits<uint64_t>::max();
    if (increment > maximum - value) {
        return maximum;
    }
    return value + increment;
}

uint64_t TamagotchiModel::saturatingAddMinutes(uint64_t age, uint64_t minuteSteps) {
    const uint64_t maximum = std::numeric_limits<uint64_t>::max();
    const uint64_t stepSeconds = TamagotchiConstants::SIMULATION_STEP_SECONDS;
    if (minuteSteps > (maximum - age) / stepSeconds) {
        return maximum;
    }
    return age + minuteSteps * stepSeconds;
}

bool TamagotchiModel::canFastForward(const TamagotchiState& state) {
    return state.stage == TamagotchiStage::Adult && state.sick && state.hunger == 0 &&
           state.happiness == 0 && state.health == 0 && state.energy == 0 &&
           state.hygiene == 0;
}
