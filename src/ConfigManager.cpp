#include "ConfigManager.h"
#include "SystemController.h"
#include <ArduinoJson.h>

ConfigManager::ConfigManager() {
    // Constructor
}

ConfigManager::ConfigManager(EventQueue& eventQueue) {
    _eventQueue = &eventQueue;
}

void ConfigManager::setEventQueue(EventQueue* queue) {
    _eventQueue = queue;
}

void ConfigManager::begin() {
    // Initialize preferences
    _preferences.begin(_namespace, false);
    _insightsPrefs.begin(_insightsNamespace, false);
    _cardPrefs.begin(_cardNamespace, false);
    
    // Check initial API configuration state
    updateApiConfigurationState();
}

// Private helper to check and update API configuration state
void ConfigManager::updateApiConfigurationState() {
    if (!_preferences.isKey(_teamIdKey) || getTeamId() == NO_TEAM_ID) {
        SystemController::setApiState(ApiState::API_AWAITING_CONFIG);
        return;
    }
    
    if (!_preferences.isKey(_apiKeyKey) || getApiKey().isEmpty()) {
        SystemController::setApiState(ApiState::API_AWAITING_CONFIG);
        return;
    }
    
    // Both team ID and API key are set
    SystemController::setApiState(ApiState::API_CONFIGURED);
}

// Helper method to commit changes to flash
void ConfigManager::commit() {
    _preferences.end();
    _insightsPrefs.end();
    _cardPrefs.end();
    
    _preferences.begin(_namespace, false);
    _insightsPrefs.begin(_insightsNamespace, false);
    _cardPrefs.begin(_cardNamespace, false);
}

bool ConfigManager::saveWiFiCredentials(const String& ssid, const String& password) {
    if (ssid.length() == 0 || ssid.length() > MAX_SSID_LENGTH) {
        return false;
    }

    if (password.length() > MAX_PASSWORD_LENGTH) {
        return false;
    }

    // Save credentials
    _preferences.putString(_ssidKey, ssid);
    _preferences.putString(_passwordKey, password);
    _preferences.putBool(_hasCredentialsKey, true);
    
    // Commit changes
    commit();
    
    // Publish event if event queue is available
    if (_eventQueue != nullptr) {
        _eventQueue->publishEvent(EventType::WIFI_CREDENTIALS_FOUND, "");
    }
    
    return true;
}

bool ConfigManager::getWiFiCredentials(String& ssid, String& password) {
    if (!hasWiFiCredentials()) {
        return false;
    }

    // Retrieve credentials
    ssid = _preferences.getString(_ssidKey, "");
    password = _preferences.getString(_passwordKey, "");
    
    return true;
}

void ConfigManager::clearWiFiCredentials() {
    _preferences.remove(_ssidKey);
    _preferences.remove(_passwordKey);
    _preferences.putBool(_hasCredentialsKey, false);
    
    // Commit changes
    commit();
    
    // Publish event if event queue is available
    if (_eventQueue != nullptr) {
        _eventQueue->publishEvent(EventType::NEED_WIFI_CREDENTIALS, "");
    }
}

bool ConfigManager::hasWiFiCredentials() {
    return _preferences.getBool(_hasCredentialsKey, false);
}

bool ConfigManager::checkWiFiCredentialsAndPublish() {
    bool hasCredentials = hasWiFiCredentials();
    
    if (_eventQueue != nullptr) {
        if (hasCredentials) {
            _eventQueue->publishEvent(EventType::WIFI_CREDENTIALS_FOUND, "");
        } else {
            _eventQueue->publishEvent(EventType::NEED_WIFI_CREDENTIALS, "");
        }
    }
    
    return hasCredentials;
}


void ConfigManager::setTeamId(int teamId) {
    _preferences.putInt(_teamIdKey, teamId);
    
    // Commit changes
    commit();
    
    updateApiConfigurationState();
}

int ConfigManager::getTeamId() {
    if (!_preferences.isKey(_teamIdKey)) {
        return NO_TEAM_ID;
    }
    return _preferences.getInt(_teamIdKey);
}

void ConfigManager::setRegion(String region) {
    _preferences.putString(_regionKey, region);
    
    // Commit changes
    commit();
    
    updateApiConfigurationState();
}

String ConfigManager::getRegion() {
    if (!_preferences.isKey(_regionKey)) {
        return "us";
    }
    return _preferences.getString(_regionKey);
}

void ConfigManager::clearTeamId() {
    _preferences.remove(_teamIdKey);
    
    // Commit changes
    commit();
    
    SystemController::setApiState(ApiState::API_AWAITING_CONFIG);
}

bool ConfigManager::setApiKey(const String& apiKey) {
    if (apiKey.length() == 0 || apiKey.length() > MAX_API_KEY_LENGTH) {
        SystemController::setApiState(ApiState::API_CONFIG_INVALID);
        return false;
    }

    _preferences.putString(_apiKeyKey, apiKey);
    
    // Commit changes
    commit();
    
    updateApiConfigurationState();
    return true;
}

String ConfigManager::getApiKey() {
    return _preferences.getString(_apiKeyKey, "");
}

void ConfigManager::clearApiKey() {
    _preferences.remove(_apiKeyKey);
    
    // Commit changes
    commit();
    
    SystemController::setApiState(ApiState::API_AWAITING_CONFIG);
}

std::vector<CardConfig> ConfigManager::getCardConfigs() {
    std::vector<CardConfig> configs;
    
    // Check if the key exists first to avoid error logs
    if (!_cardPrefs.isKey("config_list")) {
        return configs; // Return empty vector if no card config stored yet
    }
    
    // Get JSON string from preferences
    String jsonString = _cardPrefs.getString("config_list", "[]");
    
    // Parse JSON
    DynamicJsonDocument doc(CARD_CONFIG_JSON_CAPACITY_BYTES);
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error || doc.overflowed() || !doc.is<JsonArray>()) {
        Serial.printf("Failed to parse card configs JSON: %s\n", error.c_str());
        return configs; // Return empty vector on parse error
    }
    
    // NVS is an untrusted ingestion boundary. Keep valid legacy ordering exactly
    // as stored, but never reinterpret malformed values as an INSIGHT card.
    JsonArray array = doc.as<JsonArray>();
    for (size_t index = 0; index < array.size(); ++index) {
        JsonVariant value = array[index];
        if (!value.is<JsonObject>()) {
            Serial.printf("Skipping stored card config at index %u: object\n",
                          static_cast<unsigned>(index));
            continue;
        }

        JsonObject obj = value.as<JsonObject>();
        JsonVariant typeValue = obj["type"];
        if (!typeValue.is<const char*>()) {
            Serial.printf("Skipping stored card config at index %u: type\n",
                          static_cast<unsigned>(index));
            continue;
        }

        CardType type;
        if (!tryStringToCardType(String(typeValue.as<const char*>()), type)) {
            Serial.printf("Skipping stored card config at index %u: unknown type\n",
                          static_cast<unsigned>(index));
            continue;
        }

        JsonVariant orderValue = obj["order"];
        if (!orderValue.is<int>()) {
            Serial.printf("Skipping stored card config at index %u: order\n",
                          static_cast<unsigned>(index));
            continue;
        }

        String configValue;
        if (obj.containsKey("config")) {
            JsonVariant storedConfig = obj["config"];
            if (!storedConfig.is<const char*>()) {
                Serial.printf("Skipping stored card config at index %u: config\n",
                              static_cast<unsigned>(index));
                continue;
            }
            configValue = storedConfig.as<const char*>();
            if (configValue.length() > MAX_CARD_FIELD_BYTES) {
                Serial.printf("Skipping stored card config at index %u: config length\n",
                              static_cast<unsigned>(index));
                continue;
            }
        }

        String nameValue;
        if (obj.containsKey("name")) {
            JsonVariant storedName = obj["name"];
            if (!storedName.is<const char*>()) {
                Serial.printf("Skipping stored card config at index %u: name\n",
                              static_cast<unsigned>(index));
                continue;
            }
            nameValue = storedName.as<const char*>();
            if (nameValue.length() > MAX_CARD_FIELD_BYTES) {
                Serial.printf("Skipping stored card config at index %u: name length\n",
                              static_cast<unsigned>(index));
                continue;
            }
        }

        configs.emplace_back(type, configValue, orderValue.as<int>(), nameValue);
    }
    
    return configs;
}

bool ConfigManager::saveCardConfigs(const std::vector<CardConfig>& configs) {
    if (configs.size() > MAX_CONFIGURED_CARDS) {
        Serial.println("Refusing to save too many card configurations");
        return false;
    }

    // Create JSON document
    DynamicJsonDocument doc(CARD_CONFIG_JSON_CAPACITY_BYTES);
    JsonArray array = doc.to<JsonArray>();
    if (array.isNull() || doc.overflowed()) {
        Serial.println("Failed to create card configuration JSON document");
        return false;
    }
    
    // Convert vector to JSON array
    for (const CardConfig& config : configs) {
        const String type = cardTypeToString(config.type);
        if (type == "UNKNOWN") {
            Serial.println("Refusing to save an unknown card type");
            return false;
        }

        JsonObject obj = array.createNestedObject();
        if (obj.isNull()) {
            Serial.println("Failed to create card config JSON object");
            return false;
        }

        obj["type"] = type;
        obj["config"] = config.config;
        obj["order"] = config.order;
        obj["name"] = config.name;

        if (doc.overflowed()) {
            Serial.println("Card config JSON document overflowed");
            return false;
        }
    }
    
    // Serialize to string
    String jsonString;
    const size_t serializedBytes = serializeJson(doc, jsonString);
    if (serializedBytes == 0 || serializedBytes > MAX_CARD_CONFIG_BODY_BYTES) {
        Serial.println("Failed to serialize card configs to JSON");
        return false;
    }
    
    // Save to preferences
    const size_t bytesWritten = _cardPrefs.putString("config_list", jsonString);
    if (bytesWritten != serializedBytes) {
        Serial.println("Failed to write full card configuration to storage");
        return false;
    }
    
    // Commit changes
    commit();
    
    // Publish event if event queue is available
    if (_eventQueue != nullptr) {
        if (!_eventQueue->publishEvent(EventType::CARD_CONFIG_CHANGED, "")) {
            Serial.println("Failed to publish card configuration change event");
        }
    }
    
    return true;
}
