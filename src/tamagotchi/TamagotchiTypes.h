#pragma once

#include <cstdint>
#include <type_traits>

// Persisted values are stable: append compatible values only; never renumber.
enum class TamagotchiEggType : uint8_t {
    Default = 0
};

enum class TamagotchiStage : uint8_t {
    Egg = 0,
    Child = 1,
    Adult = 2
};

enum class TamagotchiLocation : uint8_t {
    Room = 0
};

enum class TamagotchiAction : uint8_t {
    Feed = 0,
    Play = 1,
    Clean = 2,
    Rest = 3,
    Doctor = 4,
    Count = 5
};

enum class TamagotchiActionResult : uint8_t {
    Applied = 0,
    RefusedWhileEgg = 1,
    RefusedLowEnergy = 2,
    RefusedNotSick = 3,
    InvalidAction = 4
};

enum class TamagotchiModelChange : uint16_t {
    None = 0,
    Needs = 1U << 0,
    Stage = 1U << 1,
    SickState = 1U << 2,
    MessState = 1U << 3,
    SimulationTime = 1U << 4
};

constexpr TamagotchiModelChange operator|(TamagotchiModelChange left,
                                          TamagotchiModelChange right) {
    return static_cast<TamagotchiModelChange>(
        static_cast<uint16_t>(left) | static_cast<uint16_t>(right));
}

constexpr TamagotchiModelChange& operator|=(TamagotchiModelChange& changes,
                                            TamagotchiModelChange flag) {
    changes = changes | flag;
    return changes;
}

constexpr bool hasModelChange(TamagotchiModelChange changes,
                              TamagotchiModelChange flag) {
    return (static_cast<uint16_t>(changes) & static_cast<uint16_t>(flag)) ==
           static_cast<uint16_t>(flag);
}

namespace TamagotchiConstants {

constexpr uint8_t SCHEMA_VERSION = 1;
constexpr uint8_t MIN_NEED = 0;
constexpr uint8_t MAX_NEED = 100;

// All durations are measured in simulated seconds unless documented otherwise.
constexpr uint32_t SIMULATION_STEP_SECONDS = 60;
constexpr uint64_t CHILD_TO_ADULT_AGE_SECONDS = 24ULL * 60ULL * 60ULL;
constexpr uint32_t MAX_OFFLINE_CATCH_UP_SECONDS = 7UL * 24UL * 60UL * 60UL;
constexpr uint64_t MESS_INTERVAL_SECONDS = 6ULL * 60ULL * 60ULL;

constexpr uint32_t HUNGER_DECAY_INTERVAL_SECONDS = 20UL * 60UL;
constexpr uint32_t HAPPINESS_DECAY_INTERVAL_SECONDS = 45UL * 60UL;
constexpr uint32_t SICK_OR_NEGLECT_HAPPINESS_PENALTY_INTERVAL_SECONDS = 15UL * 60UL;
constexpr uint32_t ENERGY_DECAY_INTERVAL_SECONDS = 30UL * 60UL;
constexpr uint32_t HYGIENE_DECAY_INTERVAL_SECONDS = 45UL * 60UL;
constexpr uint32_t MESS_HYGIENE_PENALTY_INTERVAL_SECONDS = 15UL * 60UL;
constexpr uint32_t HEALTH_DECAY_INTERVAL_SECONDS = 60UL * 60UL;

// Need values are inclusive integers in the range MIN_NEED..MAX_NEED.
constexpr uint8_t SEVERE_NEED_THRESHOLD = 20;
constexpr uint8_t SICKNESS_HEALTH_THRESHOLD = 40;
constexpr uint8_t SICKNESS_HYGIENE_THRESHOLD = 20;

constexpr uint8_t STARTING_HUNGER = 100;
constexpr uint8_t STARTING_HAPPINESS = 100;
constexpr uint8_t STARTING_HEALTH = 100;
constexpr uint8_t STARTING_ENERGY = 100;
constexpr uint8_t STARTING_HYGIENE = 100;

// Signed action deltas are widened and clamped by the future model implementation.
constexpr int16_t FEED_HUNGER_DELTA = 35;
constexpr int16_t FEED_HAPPINESS_DELTA = 5;
constexpr int16_t FEED_HEALTH_DELTA = 3;
constexpr int16_t PLAY_HAPPINESS_DELTA = 20;
constexpr int16_t PLAY_ENERGY_DELTA = -15;
constexpr uint8_t PLAY_MINIMUM_ENERGY = 15;
constexpr uint8_t CLEAN_HYGIENE_VALUE = 100;
constexpr int16_t CLEAN_HEALTH_DELTA = 3;
constexpr int16_t REST_ENERGY_DELTA = 35;
constexpr int16_t REST_HEALTH_DELTA = 2;
constexpr uint8_t DOCTOR_MINIMUM_HEALTH = 60;

// Persistence record limits are serialized JSON bytes, excluding a C-string terminator.
constexpr uint32_t PERSISTENCE_CHECKPOINT_INTERVAL_SECONDS = 5UL * 60UL;
constexpr uint16_t MAX_SERIALIZED_STATE_BYTES = 384;
constexpr uint16_t JSON_DOCUMENT_CAPACITY_BYTES = 512;
constexpr uint64_t NO_CLOCK_EPOCH = 0; // No trusted UTC baseline is available.
constexpr uint64_t MIN_VALID_EPOCH = 1577836800ULL; // 2020-01-01 UTC
constexpr uint64_t MAX_VALID_EPOCH = 253402300799ULL; // 9999-12-31 UTC

constexpr char NVS_NAMESPACE[] = "tamagotchi";
constexpr char NVS_STATE_KEY[] = "state";

} // namespace TamagotchiConstants

struct TamagotchiState {
    uint8_t schemaVersion = TamagotchiConstants::SCHEMA_VERSION;
    TamagotchiEggType eggType = TamagotchiEggType::Default;
    TamagotchiStage stage = TamagotchiStage::Egg;
    TamagotchiLocation location = TamagotchiLocation::Room;
    uint64_t ageSeconds = 0;
    uint8_t hunger = TamagotchiConstants::STARTING_HUNGER;
    uint8_t happiness = TamagotchiConstants::STARTING_HAPPINESS;
    uint8_t health = TamagotchiConstants::STARTING_HEALTH;
    uint8_t energy = TamagotchiConstants::STARTING_ENERGY;
    uint8_t hygiene = TamagotchiConstants::STARTING_HYGIENE;
    bool sick = false;
    bool mess = false;
    uint64_t nextMessAtAgeSeconds = 0;
    uint8_t simulationRemainderSeconds = 0;
    uint64_t lastUpdatedEpoch = TamagotchiConstants::NO_CLOCK_EPOCH;
};

static_assert(std::is_same<std::underlying_type<TamagotchiEggType>::type, uint8_t>::value,
              "TamagotchiEggType must use uint8_t");
static_assert(std::is_same<std::underlying_type<TamagotchiStage>::type, uint8_t>::value,
              "TamagotchiStage must use uint8_t");
static_assert(std::is_same<std::underlying_type<TamagotchiLocation>::type, uint8_t>::value,
              "TamagotchiLocation must use uint8_t");
static_assert(std::is_same<std::underlying_type<TamagotchiAction>::type, uint8_t>::value,
              "TamagotchiAction must use uint8_t");
static_assert(std::is_same<std::underlying_type<TamagotchiActionResult>::type, uint8_t>::value,
              "TamagotchiActionResult must use uint8_t");
static_assert(std::is_same<std::underlying_type<TamagotchiModelChange>::type, uint16_t>::value,
              "TamagotchiModelChange must use uint16_t");

static_assert(std::is_standard_layout<TamagotchiState>::value,
              "TamagotchiState must remain standard-layout");
static_assert(std::is_trivially_copyable<TamagotchiState>::value,
              "TamagotchiState must remain trivially copyable");
static_assert(sizeof(TamagotchiState) <= 64,
              "TamagotchiState must fit within the 64-byte state budget");

static_assert(TamagotchiConstants::HUNGER_DECAY_INTERVAL_SECONDS %
                      TamagotchiConstants::SIMULATION_STEP_SECONDS ==
                  0 &&
                  TamagotchiConstants::HAPPINESS_DECAY_INTERVAL_SECONDS %
                          TamagotchiConstants::SIMULATION_STEP_SECONDS ==
                      0 &&
                  TamagotchiConstants::SICK_OR_NEGLECT_HAPPINESS_PENALTY_INTERVAL_SECONDS %
                          TamagotchiConstants::SIMULATION_STEP_SECONDS ==
                      0 &&
                  TamagotchiConstants::ENERGY_DECAY_INTERVAL_SECONDS %
                          TamagotchiConstants::SIMULATION_STEP_SECONDS ==
                      0 &&
                  TamagotchiConstants::HYGIENE_DECAY_INTERVAL_SECONDS %
                          TamagotchiConstants::SIMULATION_STEP_SECONDS ==
                      0 &&
                  TamagotchiConstants::MESS_HYGIENE_PENALTY_INTERVAL_SECONDS %
                          TamagotchiConstants::SIMULATION_STEP_SECONDS ==
                      0 &&
                  TamagotchiConstants::HEALTH_DECAY_INTERVAL_SECONDS %
                          TamagotchiConstants::SIMULATION_STEP_SECONDS ==
                      0,
              "Every decay interval must align to simulation steps");

static_assert(TamagotchiConstants::STARTING_HUNGER >= TamagotchiConstants::MIN_NEED &&
                      TamagotchiConstants::STARTING_HUNGER <= TamagotchiConstants::MAX_NEED &&
                  TamagotchiConstants::STARTING_HAPPINESS >= TamagotchiConstants::MIN_NEED &&
                      TamagotchiConstants::STARTING_HAPPINESS <= TamagotchiConstants::MAX_NEED &&
                  TamagotchiConstants::STARTING_HEALTH >= TamagotchiConstants::MIN_NEED &&
                      TamagotchiConstants::STARTING_HEALTH <= TamagotchiConstants::MAX_NEED &&
                  TamagotchiConstants::STARTING_ENERGY >= TamagotchiConstants::MIN_NEED &&
                      TamagotchiConstants::STARTING_ENERGY <= TamagotchiConstants::MAX_NEED &&
                  TamagotchiConstants::STARTING_HYGIENE >= TamagotchiConstants::MIN_NEED &&
                      TamagotchiConstants::STARTING_HYGIENE <= TamagotchiConstants::MAX_NEED &&
                  TamagotchiConstants::SEVERE_NEED_THRESHOLD >= TamagotchiConstants::MIN_NEED &&
                      TamagotchiConstants::SEVERE_NEED_THRESHOLD <= TamagotchiConstants::MAX_NEED &&
                  TamagotchiConstants::SICKNESS_HEALTH_THRESHOLD >=
                          TamagotchiConstants::MIN_NEED &&
                      TamagotchiConstants::SICKNESS_HEALTH_THRESHOLD <=
                          TamagotchiConstants::MAX_NEED &&
                  TamagotchiConstants::SICKNESS_HYGIENE_THRESHOLD >=
                          TamagotchiConstants::MIN_NEED &&
                      TamagotchiConstants::SICKNESS_HYGIENE_THRESHOLD <=
                          TamagotchiConstants::MAX_NEED &&
                  TamagotchiConstants::PLAY_MINIMUM_ENERGY >= TamagotchiConstants::MIN_NEED &&
                      TamagotchiConstants::PLAY_MINIMUM_ENERGY <= TamagotchiConstants::MAX_NEED &&
                  TamagotchiConstants::CLEAN_HYGIENE_VALUE >= TamagotchiConstants::MIN_NEED &&
                      TamagotchiConstants::CLEAN_HYGIENE_VALUE <= TamagotchiConstants::MAX_NEED &&
                  TamagotchiConstants::DOCTOR_MINIMUM_HEALTH >= TamagotchiConstants::MIN_NEED &&
                      TamagotchiConstants::DOCTOR_MINIMUM_HEALTH <= TamagotchiConstants::MAX_NEED,
              "Starting needs and action thresholds must be valid need values");

static_assert(TamagotchiConstants::CHILD_TO_ADULT_AGE_SECONDS != 0 &&
                  TamagotchiConstants::CHILD_TO_ADULT_AGE_SECONDS %
                          TamagotchiConstants::SIMULATION_STEP_SECONDS ==
                      0 &&
                  TamagotchiConstants::MESS_INTERVAL_SECONDS != 0 &&
                  TamagotchiConstants::MESS_INTERVAL_SECONDS %
                          TamagotchiConstants::SIMULATION_STEP_SECONDS ==
                      0,
              "Lifecycle thresholds must align to simulation steps");
static_assert(TamagotchiConstants::MAX_OFFLINE_CATCH_UP_SECONDS %
                      TamagotchiConstants::SIMULATION_STEP_SECONDS ==
                  0,
              "Offline catch-up must align to simulation steps");
static_assert(sizeof(TamagotchiConstants::NVS_NAMESPACE) - 1 <= 15,
              "NVS namespace must fit Preferences limits");
static_assert(sizeof(TamagotchiConstants::NVS_STATE_KEY) - 1 <= 15,
              "NVS key must fit Preferences limits");
