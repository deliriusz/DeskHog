#pragma once

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

enum class JsonArrayParseResult : uint8_t {
    Success,
    InvalidJson,
    InvalidRoot
};

inline bool isJsonEnvelopeWhitespace(char character) {
    return character == ' ' || character == '\t' || character == '\r' || character == '\n';
}

/**
 * @brief Ensures an array root is followed only by JSON whitespace.
 *
 * ArduinoJson validates the array grammar, but accepts trailing data after a
 * container root. This bounded envelope check closes that ingestion gap.
 */
inline bool isSingleJsonArray(const uint8_t* data, size_t length) {
    if (data == nullptr) {
        return false;
    }

    size_t index = 0;
    while (index < length && isJsonEnvelopeWhitespace(static_cast<char>(data[index]))) {
        ++index;
    }

    if (index == length || data[index] != '[') {
        return false;
    }

    size_t nesting = 0;
    bool inString = false;
    bool escaped = false;

    for (; index < length; ++index) {
        const char character = static_cast<char>(data[index]);

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
                if (character != ']') {
                    return false;
                }
                ++index;
                break;
            }
        }
    }

    if (inString || nesting != 0) {
        return false;
    }

    while (index < length) {
        if (!isJsonEnvelopeWhitespace(static_cast<char>(data[index]))) {
            return false;
        }
        ++index;
    }

    return true;
}

/**
 * @brief Safely parses a JSON array from a mutable zero-copy buffer.
 *
 * ArduinoJson mutates mutable input while parsing. Capture the envelope result
 * first so trailing-data validation always examines the original request bytes.
 */
inline JsonArrayParseResult parseMutableJsonArray(
    DynamicJsonDocument& document,
    uint8_t* data,
    size_t length
) {
    const bool hasValidEnvelope = isSingleJsonArray(data, length);
    const DeserializationError error = deserializeJson(document, data, length);

    if (error || document.overflowed()) {
        return JsonArrayParseResult::InvalidJson;
    }
    if (!document.is<JsonArray>()) {
        return JsonArrayParseResult::InvalidRoot;
    }
    if (!hasValidEnvelope) {
        return JsonArrayParseResult::InvalidJson;
    }

    return JsonArrayParseResult::Success;
}
