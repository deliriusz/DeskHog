#include "tamagotchi/TamagotchiStateStore.h"

#include <ArduinoJson.h>

#include <limits>

namespace {

enum class FieldReadStatus : uint8_t {
    Ok,
    Type,
    Range
};

enum class StateValidation : uint8_t {
    Valid,
    Range,
    Invariant
};

enum class DecodeStatus : uint8_t {
    Ok,
    FieldType,
    Range,
    Invariant
};

void logCategory(const char* category) {
    Serial.print("TamagotchiStore: ");
    Serial.println(category);
}

void logUnsupportedVersion(uint64_t schemaVersion) {
    Serial.printf("TamagotchiStore: unsupported-version %llu\n",
                  static_cast<unsigned long long>(schemaVersion));
}

bool isJsonWhitespace(char character) {
    return character == ' ' || character == '\t' || character == '\r' || character == '\n';
}

// ArduinoJson's DOM parser keeps the final duplicate-key value. It is still decoded and
// validated exactly like any other value. This bounded envelope check also rejects a second
// trailing JSON value, which ArduinoJson otherwise accepts after an object root.
bool hasSingleJsonObject(const char* record, size_t length) {
    size_t index = 0;
    while (index < length && isJsonWhitespace(record[index])) {
        ++index;
    }

    if (index == length || record[index] != '{') {
        return false;
    }

    uint16_t nesting = 0;
    bool inString = false;
    bool escaped = false;

    for (; index < length; ++index) {
        const char character = record[index];

        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (character == '\\') {
                escaped = true;
            } else if (character == '"') {
                inString = false;
            }
            continue;
        }

        if (character == '"') {
            inString = true;
        } else if (character == '{' || character == '[') {
            ++nesting;
        } else if (character == '}' || character == ']') {
            if (nesting == 0) {
                return false;
            }

            --nesting;
            if (nesting == 0) {
                ++index;
                break;
            }
        }
    }

    if (inString || nesting != 0) {
        return false;
    }

    while (index < length) {
        if (!isJsonWhitespace(record[index])) {
            return false;
        }
        ++index;
    }

    return true;
}

FieldReadStatus readUnsignedField(JsonObjectConst object, const char* key, uint64_t maximum,
                                  uint64_t& value) {
    if (!object.containsKey(key)) {
        return FieldReadStatus::Type;
    }

    const JsonVariantConst field = object[key];
    if (!field.is<uint64_t>()) {
        return FieldReadStatus::Type;
    }

    const uint64_t parsed = field.as<uint64_t>();
    if (parsed > maximum) {
        return FieldReadStatus::Range;
    }

    value = parsed;
    return FieldReadStatus::Ok;
}

FieldReadStatus readBooleanField(JsonObjectConst object, const char* key, bool& value) {
    if (!object.containsKey(key)) {
        return FieldReadStatus::Type;
    }

    const JsonVariantConst field = object[key];
    if (!field.is<bool>()) {
        return FieldReadStatus::Type;
    }

    value = field.as<bool>();
    return FieldReadStatus::Ok;
}

StateValidation validateState(const TamagotchiState& state) {
    if (state.schemaVersion != TamagotchiConstants::SCHEMA_VERSION ||
        state.eggType != TamagotchiEggType::Default ||
        state.location != TamagotchiLocation::Room ||
        (state.stage != TamagotchiStage::Egg && state.stage != TamagotchiStage::Child &&
         state.stage != TamagotchiStage::Adult)) {
        return StateValidation::Invariant;
    }

    if (state.hunger > TamagotchiConstants::MAX_NEED ||
        state.happiness > TamagotchiConstants::MAX_NEED ||
        state.health > TamagotchiConstants::MAX_NEED ||
        state.energy > TamagotchiConstants::MAX_NEED ||
        state.hygiene > TamagotchiConstants::MAX_NEED || state.simulationRemainderSeconds >= 60 ||
        (state.lastUpdatedEpoch != TamagotchiConstants::NO_CLOCK_EPOCH &&
         (state.lastUpdatedEpoch < TamagotchiConstants::MIN_VALID_EPOCH ||
          state.lastUpdatedEpoch > TamagotchiConstants::MAX_VALID_EPOCH))) {
        return StateValidation::Range;
    }

    if (state.stage == TamagotchiStage::Egg) {
        if (state.ageSeconds != 0 || state.simulationRemainderSeconds != 0 || state.sick ||
            state.mess || state.nextMessAtAgeSeconds != 0) {
            return StateValidation::Invariant;
        }
        return StateValidation::Valid;
    }

    if (state.stage == TamagotchiStage::Child &&
        (state.ageSeconds >= TamagotchiConstants::CHILD_TO_ADULT_AGE_SECONDS ||
         state.nextMessAtAgeSeconds == 0)) {
        return StateValidation::Invariant;
    }

    if (state.stage == TamagotchiStage::Adult &&
        (state.ageSeconds < TamagotchiConstants::CHILD_TO_ADULT_AGE_SECONDS ||
         state.nextMessAtAgeSeconds == 0)) {
        return StateValidation::Invariant;
    }

    if ((!state.mess && state.nextMessAtAgeSeconds <= state.ageSeconds) ||
        (state.mess && state.nextMessAtAgeSeconds > state.ageSeconds)) {
        return StateValidation::Invariant;
    }

    return StateValidation::Valid;
}

DecodeStatus decodeV1(JsonObjectConst object, TamagotchiState& state) {
    TamagotchiState candidate;
    uint64_t value = 0;

    const auto readUnsigned = [&](const char* key, uint64_t maximum) {
        return readUnsignedField(object, key, maximum, value);
    };

    FieldReadStatus readStatus =
        readUnsigned("schemaVersion", TamagotchiConstants::SCHEMA_VERSION);
    if (readStatus != FieldReadStatus::Ok) {
        return readStatus == FieldReadStatus::Type ? DecodeStatus::FieldType : DecodeStatus::Range;
    }
    candidate.schemaVersion = static_cast<uint8_t>(value);

    readStatus = readUnsigned("eggType", static_cast<uint8_t>(TamagotchiEggType::Default));
    if (readStatus != FieldReadStatus::Ok) {
        return readStatus == FieldReadStatus::Type ? DecodeStatus::FieldType : DecodeStatus::Range;
    }
    candidate.eggType = static_cast<TamagotchiEggType>(value);

    readStatus = readUnsigned("stage", static_cast<uint8_t>(TamagotchiStage::Adult));
    if (readStatus != FieldReadStatus::Ok) {
        return readStatus == FieldReadStatus::Type ? DecodeStatus::FieldType : DecodeStatus::Range;
    }
    candidate.stage = static_cast<TamagotchiStage>(value);

    readStatus = readUnsigned("location", static_cast<uint8_t>(TamagotchiLocation::Room));
    if (readStatus != FieldReadStatus::Ok) {
        return readStatus == FieldReadStatus::Type ? DecodeStatus::FieldType : DecodeStatus::Range;
    }
    candidate.location = static_cast<TamagotchiLocation>(value);

    readStatus = readUnsigned("ageSeconds", std::numeric_limits<uint64_t>::max());
    if (readStatus != FieldReadStatus::Ok) {
        return readStatus == FieldReadStatus::Type ? DecodeStatus::FieldType : DecodeStatus::Range;
    }
    candidate.ageSeconds = value;

    readStatus = readUnsigned("hunger", TamagotchiConstants::MAX_NEED);
    if (readStatus != FieldReadStatus::Ok) {
        return readStatus == FieldReadStatus::Type ? DecodeStatus::FieldType : DecodeStatus::Range;
    }
    candidate.hunger = static_cast<uint8_t>(value);

    readStatus = readUnsigned("happiness", TamagotchiConstants::MAX_NEED);
    if (readStatus != FieldReadStatus::Ok) {
        return readStatus == FieldReadStatus::Type ? DecodeStatus::FieldType : DecodeStatus::Range;
    }
    candidate.happiness = static_cast<uint8_t>(value);

    readStatus = readUnsigned("health", TamagotchiConstants::MAX_NEED);
    if (readStatus != FieldReadStatus::Ok) {
        return readStatus == FieldReadStatus::Type ? DecodeStatus::FieldType : DecodeStatus::Range;
    }
    candidate.health = static_cast<uint8_t>(value);

    readStatus = readUnsigned("energy", TamagotchiConstants::MAX_NEED);
    if (readStatus != FieldReadStatus::Ok) {
        return readStatus == FieldReadStatus::Type ? DecodeStatus::FieldType : DecodeStatus::Range;
    }
    candidate.energy = static_cast<uint8_t>(value);

    readStatus = readUnsigned("hygiene", TamagotchiConstants::MAX_NEED);
    if (readStatus != FieldReadStatus::Ok) {
        return readStatus == FieldReadStatus::Type ? DecodeStatus::FieldType : DecodeStatus::Range;
    }
    candidate.hygiene = static_cast<uint8_t>(value);

    readStatus = readBooleanField(object, "sick", candidate.sick);
    if (readStatus != FieldReadStatus::Ok) {
        return DecodeStatus::FieldType;
    }

    readStatus = readBooleanField(object, "mess", candidate.mess);
    if (readStatus != FieldReadStatus::Ok) {
        return DecodeStatus::FieldType;
    }

    readStatus = readUnsigned("nextMessAtAgeSeconds", std::numeric_limits<uint64_t>::max());
    if (readStatus != FieldReadStatus::Ok) {
        return readStatus == FieldReadStatus::Type ? DecodeStatus::FieldType : DecodeStatus::Range;
    }
    candidate.nextMessAtAgeSeconds = value;

    readStatus = readUnsigned("simulationRemainderSeconds", 59);
    if (readStatus != FieldReadStatus::Ok) {
        return readStatus == FieldReadStatus::Type ? DecodeStatus::FieldType : DecodeStatus::Range;
    }
    candidate.simulationRemainderSeconds = static_cast<uint8_t>(value);

    readStatus = readUnsigned("lastUpdatedEpoch", std::numeric_limits<uint64_t>::max());
    if (readStatus != FieldReadStatus::Ok) {
        return readStatus == FieldReadStatus::Type ? DecodeStatus::FieldType : DecodeStatus::Range;
    }
    candidate.lastUpdatedEpoch = value;

    const StateValidation validation = validateState(candidate);
    if (validation == StateValidation::Range) {
        return DecodeStatus::Range;
    }
    if (validation != StateValidation::Valid) {
        return DecodeStatus::Invariant;
    }

    state = candidate;
    return DecodeStatus::Ok;
}

bool replaceWithFreshState(TamagotchiStateStore& store, TamagotchiLoadResult& result,
                           TamagotchiLoadOrigin origin) {
    result.origin = origin;
    result.persisted = store.save(result.state);
    if (!result.persisted) {
        logCategory("replacement-write");
    }
    return result.persisted;
}

} // namespace

TamagotchiStateStore::~TamagotchiStateStore() {
    end();
}

bool TamagotchiStateStore::begin() {
    if (ready) {
        return true;
    }

    ready = preferences.begin(TamagotchiConstants::NVS_NAMESPACE, false);
    if (!ready) {
        logCategory("open");
    }
    return ready;
}

void TamagotchiStateStore::end() {
    if (!ready) {
        return;
    }

    preferences.end();
    ready = false;
}

bool TamagotchiStateStore::isReady() const {
    return ready;
}

TamagotchiLoadResult TamagotchiStateStore::load() {
    TamagotchiLoadResult result;

    if (!ready) {
        logCategory("open");
        return result;
    }

    if (!preferences.isKey(TamagotchiConstants::NVS_STATE_KEY)) {
        logCategory("missing");
        replaceWithFreshState(*this, result, TamagotchiLoadOrigin::Missing);
        return result;
    }

    if (preferences.getType(TamagotchiConstants::NVS_STATE_KEY) != PT_STR) {
        logCategory("wrong-type");
        replaceWithFreshState(*this, result, TamagotchiLoadOrigin::InvalidRecord);
        return result;
    }

    char record[TamagotchiConstants::MAX_SERIALIZED_STATE_BYTES + 1] = {};
    const size_t storedLength = preferences.getString(TamagotchiConstants::NVS_STATE_KEY, record,
                                                       sizeof(record));
    if (storedLength == 0 || storedLength > sizeof(record) || record[storedLength - 1] != '\0') {
        logCategory("read-or-size");
        replaceWithFreshState(*this, result, TamagotchiLoadOrigin::InvalidRecord);
        return result;
    }

    const size_t jsonLength = storedLength - 1;
    if (jsonLength == 0 || jsonLength >= TamagotchiConstants::MAX_SERIALIZED_STATE_BYTES) {
        logCategory("read-or-size");
        replaceWithFreshState(*this, result, TamagotchiLoadOrigin::InvalidRecord);
        return result;
    }

    if (!hasSingleJsonObject(record, jsonLength)) {
        logCategory("parse-or-overflow");
        replaceWithFreshState(*this, result, TamagotchiLoadOrigin::InvalidRecord);
        return result;
    }

    StaticJsonDocument<TamagotchiConstants::JSON_DOCUMENT_CAPACITY_BYTES> document;
    const DeserializationError error = deserializeJson(document, record, jsonLength);
    if (error || document.overflowed() || !document.is<JsonObject>()) {
        logCategory("parse-or-overflow");
        replaceWithFreshState(*this, result, TamagotchiLoadOrigin::InvalidRecord);
        return result;
    }

    const JsonObjectConst object = document.as<JsonObjectConst>();
    uint64_t schemaVersion = 0;
    const FieldReadStatus schemaStatus =
        readUnsignedField(object, "schemaVersion", std::numeric_limits<uint64_t>::max(), schemaVersion);
    if (schemaStatus == FieldReadStatus::Type) {
        logCategory("field-type");
        replaceWithFreshState(*this, result, TamagotchiLoadOrigin::InvalidRecord);
        return result;
    }
    if (schemaStatus == FieldReadStatus::Range) {
        logCategory("range");
        replaceWithFreshState(*this, result, TamagotchiLoadOrigin::InvalidRecord);
        return result;
    }

    switch (schemaVersion) {
    case TamagotchiConstants::SCHEMA_VERSION: {
        TamagotchiState candidate;
        const DecodeStatus decodeStatus = decodeV1(object, candidate);
        if (decodeStatus == DecodeStatus::FieldType) {
            logCategory("field-type");
        } else if (decodeStatus == DecodeStatus::Range) {
            logCategory("range");
        } else if (decodeStatus == DecodeStatus::Invariant) {
            logCategory("invariant");
        }

        if (decodeStatus != DecodeStatus::Ok) {
            replaceWithFreshState(*this, result, TamagotchiLoadOrigin::InvalidRecord);
            return result;
        }

        result.state = candidate;
        result.origin = TamagotchiLoadOrigin::StoredV1;
        result.persisted = true;
        return result;
    }

    default:
        logUnsupportedVersion(schemaVersion);
        replaceWithFreshState(*this, result, TamagotchiLoadOrigin::UnsupportedSchema);
        return result;
    }
}

bool TamagotchiStateStore::save(const TamagotchiState& state) {
    if (!ready) {
        logCategory("open");
        return false;
    }

    const StateValidation validation = validateState(state);
    if (validation == StateValidation::Range) {
        logCategory("range");
        return false;
    }
    if (validation != StateValidation::Valid) {
        logCategory("invariant");
        return false;
    }

    StaticJsonDocument<TamagotchiConstants::JSON_DOCUMENT_CAPACITY_BYTES> document;
    document["schemaVersion"] = state.schemaVersion;
    document["eggType"] = static_cast<uint8_t>(state.eggType);
    document["stage"] = static_cast<uint8_t>(state.stage);
    document["location"] = static_cast<uint8_t>(state.location);
    document["ageSeconds"] = state.ageSeconds;
    document["hunger"] = state.hunger;
    document["happiness"] = state.happiness;
    document["health"] = state.health;
    document["energy"] = state.energy;
    document["hygiene"] = state.hygiene;
    document["sick"] = state.sick;
    document["mess"] = state.mess;
    document["nextMessAtAgeSeconds"] = state.nextMessAtAgeSeconds;
    document["simulationRemainderSeconds"] = state.simulationRemainderSeconds;
    document["lastUpdatedEpoch"] = state.lastUpdatedEpoch;

    if (document.overflowed()) {
        logCategory("serialize");
        return false;
    }

    const size_t measuredLength = measureJson(document);
    if (measuredLength == 0 ||
        measuredLength >= TamagotchiConstants::MAX_SERIALIZED_STATE_BYTES) {
        logCategory("serialize");
        return false;
    }

    char record[TamagotchiConstants::MAX_SERIALIZED_STATE_BYTES] = {};
    const size_t serializedLength = serializeJson(document, record, sizeof(record));
    if (serializedLength != measuredLength ||
        serializedLength >= TamagotchiConstants::MAX_SERIALIZED_STATE_BYTES) {
        logCategory("serialize");
        return false;
    }

    const size_t writtenLength = preferences.putString(TamagotchiConstants::NVS_STATE_KEY, record);
    if (writtenLength != serializedLength) {
        logCategory("write");
        return false;
    }

    return true;
}
