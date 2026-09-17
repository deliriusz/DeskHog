#include "ui/TamagotchiCard.h"

#include <Arduino.h>

#include <cstdio>
#include <limits>

#include "Style.h"
#include "sprites/sprites.h"

#if LVGL_VERSION_MAJOR != 9 || !LV_VERSION_CHECK(9, 2, 0)
#error "TamagotchiCard requires a reviewed LVGL 9.x version at or above 9.2"
#endif

namespace {

constexpr uint16_t kCardWidth = 233;
constexpr uint16_t kCardHeight = 135;
constexpr uint32_t kAnimationCheckIntervalMs = 50;
constexpr uint32_t kModelUpdateIntervalMs = 1000;

constexpr uint32_t kEggDurationMs = 900;
constexpr uint32_t kIdleDurationMs = 900;
constexpr uint32_t kSickDurationMs = 700;
constexpr uint32_t kActionDurationMs = 750;
constexpr uint32_t kRestDurationMs = 900;
constexpr uint32_t kDoctorDurationMs = 900;
constexpr uint32_t kEvolveDurationMs = 1200;

constexpr uint32_t kResultDurationMs = 1600;
constexpr uint32_t kRefusalResultDurationMs = 1800;

constexpr uint32_t kRoomColor = 0x17212B;
constexpr uint32_t kFloorColor = 0x40505C;
constexpr uint32_t kFooterColor = 0x090D10;
constexpr uint32_t kSeparatorColor = 0x26313A;
constexpr uint32_t kWarningColor = 0xF2C14E;
constexpr uint32_t kErrorColor = 0xFF5A5F;
constexpr uint32_t kMessColor = 0xD98C3F;
constexpr uint32_t kBarTrackColor = 0x26313A;

constexpr uint8_t kNeedCount = 5;
constexpr const char* kNeedNames[kNeedCount] = {"HU", "HA", "HP", "EN", "HY"};

uint8_t needValue(const TamagotchiState& state, uint8_t index) {
    switch (index) {
    case 0:
        return state.hunger;
    case 1:
        return state.happiness;
    case 2:
        return state.health;
    case 3:
        return state.energy;
    case 4:
        return state.hygiene;
    default:
        return 0;
    }
}

const char* stageText(TamagotchiStage stage) {
    switch (stage) {
    case TamagotchiStage::Egg:
        return "EGG";
    case TamagotchiStage::Child:
        return "CHILD";
    case TamagotchiStage::Adult:
        return "ADULT";
    default:
        return "PET?";
    }
}

const char* actionText(TamagotchiAction action) {
    switch (action) {
    case TamagotchiAction::Feed:
        return "FEED";
    case TamagotchiAction::Play:
        return "PLAY";
    case TamagotchiAction::Clean:
        return "CLEAN";
    case TamagotchiAction::Rest:
        return "REST";
    case TamagotchiAction::Doctor:
        return "DOCTOR";
    case TamagotchiAction::Count:
    default:
        return "ACTION?";
    }
}

void styleContainer(lv_obj_t* object, lv_color_t color, lv_opa_t opacity) {
    lv_obj_set_style_bg_color(object, color, 0);
    lv_obj_set_style_bg_opa(object, opacity, 0);
    lv_obj_set_style_border_width(object, 0, 0);
    lv_obj_set_style_radius(object, 0, 0);
    lv_obj_set_style_pad_all(object, 0, 0);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
}

} // namespace

TamagotchiCard::InitialSession TamagotchiCard::loadInitialSession(
    TamagotchiStateStore& stateStore) {
    const TamagotchiLoadResult loaded = stateStore.load();

    InitialSession initialSession;
    initialSession.state = loaded.state;
    initialSession.persistenceDirty = !loaded.persisted;
    initialSession.saveFailed = !loaded.persisted;
    return initialSession;
}

TamagotchiCard::TamagotchiCard(lv_obj_t* parent, TamagotchiStateStore& stateStore,
                               ClockService& clockService)
    : TamagotchiCard(parent, stateStore, clockService, loadInitialSession(stateStore)) {}

TamagotchiCard::TamagotchiCard(lv_obj_t* parent, TamagotchiStateStore& stateStore,
                               ClockService& clockService, InitialSession initialSession)
    : _stateStore(stateStore),
      _clockService(clockService),
      _model(initialSession.state),
      _persistenceDirty(initialSession.persistenceDirty),
      _saveFailed(initialSession.saveFailed),
      _lastAdvanceMillis(millis()),
      _millisRemainder(0),
      _sameBootAppliedSeconds(0),
      _pendingBaselineEpoch(initialSession.state.lastUpdatedEpoch),
      _catchUpState(OfflineCatchUpState::PendingClock),
      _timeUnknown(true),
      _lastModelUpdateMillis(_lastAdvanceMillis - kModelUpdateIntervalMs),
      _lastAnimationCheckMillis(_lastAdvanceMillis),
      _pendingRenderChanges(TamagotchiModelChange::None),
      _stageBeforeLastAdvance(initialSession.state.stage),
      _currentVisual(PetVisual::None),
      _oneShotVisual(PetVisual::None),
      _oneShotDeadlineMillis(0),
      _resultDeadlineMillis(0),
      _resultText{},
      _rendered{},
      _preparedForRemoval(false),
      _visualFaultLogged(false) {
    createUi(parent);
}

TamagotchiCard::~TamagotchiCard() {
    if (!_preparedForRemoval) {
        destroyUnownedUi();
    }
}

lv_obj_t* TamagotchiCard::getCard() const {
    return _card;
}

bool TamagotchiCard::handleButtonPress(uint8_t buttonIndex) {
    static_cast<void>(buttonIndex);
    return false;
}

bool TamagotchiCard::update() {
    if (_preparedForRemoval || !isValidObject(_card)) {
        return false;
    }

    const uint32_t nowMillis = millis();
    if (static_cast<uint32_t>(nowMillis - _lastAnimationCheckMillis) >=
        kAnimationCheckIntervalMs) {
        _lastAnimationCheckMillis = nowMillis;
        expireTimedUi(nowMillis);
    }

    if (static_cast<uint32_t>(nowMillis - _lastModelUpdateMillis) >=
        kModelUpdateIntervalMs) {
        _lastModelUpdateMillis = nowMillis;
        advanceFromMillis(nowMillis);
        tryApplyOfflineCatchUp();
        renderModel(nowMillis, false);
    }

    return true;
}

void TamagotchiCard::prepareForRemoval() {
    if (_preparedForRemoval) {
        return;
    }

    if (isValidObject(_petImage)) {
        stopPetAnimation(_petImage);
    }

    _oneShotVisual = PetVisual::None;
    _oneShotDeadlineMillis = 0;
    _resultDeadlineMillis = 0;
    _resultText[0] = '\0';
    _currentVisual = PetVisual::None;
    _preparedForRemoval = true;
    clearUiPointers();
}

bool TamagotchiCard::deadlineReached(uint32_t nowMillis, uint32_t deadlineMillis) {
    return static_cast<int32_t>(nowMillis - deadlineMillis) >= 0;
}

bool TamagotchiCard::isValidObject(const lv_obj_t* object) {
    return object != nullptr && lv_obj_is_valid(const_cast<lv_obj_t*>(object));
}

bool TamagotchiCard::createUi(lv_obj_t* parent) {
    if (!isValidObject(parent)) {
        Serial.println("Tamagotchi UI creation failed");
        return false;
    }

    _card = lv_obj_create(parent);
    if (_card == nullptr) {
        goto failed;
    }
    lv_obj_set_pos(_card, 0, 0);
    lv_obj_set_size(_card, kCardWidth, kCardHeight);
    styleContainer(_card, Style::backgroundColor(), LV_OPA_COVER);

    _header = lv_obj_create(_card);
    if (_header == nullptr) {
        goto failed;
    }
    lv_obj_set_pos(_header, 0, 0);
    lv_obj_set_size(_header, 233, 18);
    styleContainer(_header, lv_color_black(), LV_OPA_TRANSP);

    _stageLabel = lv_label_create(_header);
    if (_stageLabel == nullptr) {
        goto failed;
    }
    lv_obj_set_pos(_stageLabel, 3, 0);
    lv_obj_set_size(_stageLabel, 45, 18);
    lv_obj_set_style_text_font(_stageLabel, Style::labelFont(), 0);
    lv_obj_set_style_text_color(_stageLabel, Style::valueColor(), 0);
    lv_obj_set_style_text_align(_stageLabel, LV_TEXT_ALIGN_LEFT, 0);

    _resultLabel = lv_label_create(_header);
    if (_resultLabel == nullptr) {
        goto failed;
    }
    lv_obj_set_pos(_resultLabel, 48, 0);
    lv_obj_set_size(_resultLabel, 104, 18);
    lv_obj_set_style_text_font(_resultLabel, Style::labelFont(), 0);
    lv_obj_set_style_text_color(_resultLabel, Style::labelColor(), 0);
    lv_obj_set_style_text_align(_resultLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(_resultLabel, LV_LABEL_LONG_DOT);
    lv_label_set_text(_resultLabel, "");

    _statusLabel = lv_label_create(_header);
    if (_statusLabel == nullptr) {
        goto failed;
    }
    lv_obj_set_pos(_statusLabel, 152, 0);
    lv_obj_set_size(_statusLabel, 78, 18);
    lv_obj_set_style_text_font(_statusLabel, Style::labelFont(), 0);
    lv_obj_set_style_text_color(_statusLabel, Style::labelColor(), 0);
    lv_obj_set_style_text_align(_statusLabel, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(_statusLabel, "");

    _roomViewport = lv_obj_create(_card);
    if (_roomViewport == nullptr) {
        goto failed;
    }
    lv_obj_set_pos(_roomViewport, 3, 19);
    lv_obj_set_size(_roomViewport, 137, 89);
    styleContainer(_roomViewport, lv_color_hex(kRoomColor), LV_OPA_COVER);
    lv_obj_set_style_radius(_roomViewport, 4, 0);
    lv_obj_clear_flag(_roomViewport, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    _floorLine = lv_obj_create(_roomViewport);
    if (_floorLine == nullptr) {
        goto failed;
    }
    lv_obj_set_pos(_floorLine, 3, 81);
    lv_obj_set_size(_floorLine, 131, 2);
    styleContainer(_floorLine, lv_color_hex(kFloorColor), LV_OPA_COVER);

    _petImage = lv_animimg_create(_roomViewport);
    if (_petImage == nullptr) {
        goto failed;
    }
    if (!setPetAnimation(tamagotchi_egg_sprites, tamagotchi_egg_sprites_count,
                         kEggDurationMs, true)) {
        goto failed;
    }
    _currentVisual = PetVisual::Egg;
    lv_obj_set_pos(_petImage, 53, 33);
    lv_obj_set_size(_petImage, 32, 32);
    lv_image_set_pivot(_petImage, 16, 16);
    lv_image_set_antialias(_petImage, false);
    lv_image_set_scale(_petImage, 512);

    _messImage = lv_image_create(_roomViewport);
    if (_messImage == nullptr || tamagotchi_mess_sprites_count != 1 ||
        tamagotchi_mess_sprites[0] == nullptr) {
        goto failed;
    }
    lv_image_set_src(_messImage, tamagotchi_mess_sprites[0]);
    lv_obj_set_pos(_messImage, 105, 65);
    lv_obj_set_size(_messImage, 16, 16);
    lv_obj_add_flag(_messImage, LV_OBJ_FLAG_HIDDEN);

    _needsPanel = lv_obj_create(_card);
    if (_needsPanel == nullptr) {
        goto failed;
    }
    lv_obj_set_pos(_needsPanel, 143, 19);
    lv_obj_set_size(_needsPanel, 87, 90);
    styleContainer(_needsPanel, lv_color_black(), LV_OPA_TRANSP);

    for (uint8_t index = 0; index < kNeedCount; ++index) {
        const int16_t rowY = static_cast<int16_t>(index * 18);
        _needLabels[index] = lv_label_create(_needsPanel);
        if (_needLabels[index] == nullptr) {
            goto failed;
        }
        lv_obj_set_pos(_needLabels[index], 0, rowY);
        lv_obj_set_size(_needLabels[index], 18, 18);
        lv_obj_set_style_text_font(_needLabels[index], Style::labelFont(), 0);
        lv_obj_set_style_text_color(_needLabels[index], Style::labelColor(), 0);
        lv_obj_set_style_text_align(_needLabels[index], LV_TEXT_ALIGN_LEFT, 0);
        lv_label_set_text(_needLabels[index], kNeedNames[index]);

        _needBars[index] = lv_bar_create(_needsPanel);
        if (_needBars[index] == nullptr) {
            goto failed;
        }
        lv_obj_set_pos(_needBars[index], 21, static_cast<int16_t>(rowY + 6));
        lv_obj_set_size(_needBars[index], 64, 7);
        lv_bar_set_range(_needBars[index], 0, 100);
        lv_obj_set_style_bg_color(_needBars[index], lv_color_hex(kBarTrackColor), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(_needBars[index], LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_radius(_needBars[index], 2, LV_PART_MAIN);
        lv_obj_set_style_border_width(_needBars[index], 0, LV_PART_MAIN);
        lv_obj_set_style_bg_color(_needBars[index], Style::accentColor(),
                                  LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(_needBars[index], LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_set_style_radius(_needBars[index], 2, LV_PART_INDICATOR);
        lv_obj_set_style_border_width(_needBars[index], 0, LV_PART_INDICATOR);
    }

    _footer = lv_obj_create(_card);
    if (_footer == nullptr) {
        goto failed;
    }
    lv_obj_set_pos(_footer, 0, 110);
    lv_obj_set_size(_footer, 233, 25);
    styleContainer(_footer, lv_color_hex(kFooterColor), LV_OPA_COVER);
    lv_obj_set_style_border_side(_footer, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(_footer, 1, 0);
    lv_obj_set_style_border_color(_footer, lv_color_hex(kSeparatorColor), 0);

    _actionLabel = lv_label_create(_footer);
    if (_actionLabel == nullptr) {
        goto failed;
    }
    lv_obj_set_pos(_actionLabel, 4, 2);
    lv_obj_set_size(_actionLabel, 122, 21);
    lv_obj_set_style_text_font(_actionLabel, Style::valueFont(), 0);
    lv_obj_set_style_text_color(_actionLabel, Style::valueColor(), 0);
    lv_obj_set_style_text_align(_actionLabel, LV_TEXT_ALIGN_LEFT, 0);

    _hintLabel = lv_label_create(_footer);
    if (_hintLabel == nullptr) {
        goto failed;
    }
    lv_obj_set_pos(_hintLabel, 127, 3);
    lv_obj_set_size(_hintLabel, 102, 18);
    lv_obj_set_style_text_font(_hintLabel, Style::labelFont(), 0);
    lv_obj_set_style_text_color(_hintLabel, Style::labelColor(), 0);
    lv_obj_set_style_text_align(_hintLabel, LV_TEXT_ALIGN_RIGHT, 0);

    renderFooter(false, TamagotchiAction::Feed);
    renderModel(millis(), true);
    return true;

failed:
    Serial.println("Tamagotchi UI creation failed");
    destroyUnownedUi();
    return false;
}

void TamagotchiCard::destroyUnownedUi() {
    if (isValidObject(_petImage)) {
        stopPetAnimation(_petImage);
    }

    if (isValidObject(_card)) {
        lv_obj_delete(_card);
    }

    clearUiPointers();
    _currentVisual = PetVisual::None;
    _oneShotVisual = PetVisual::None;
    _oneShotDeadlineMillis = 0;
    _resultDeadlineMillis = 0;
}

void TamagotchiCard::clearUiPointers() {
    _card = nullptr;
    _header = nullptr;
    _stageLabel = nullptr;
    _resultLabel = nullptr;
    _statusLabel = nullptr;
    _roomViewport = nullptr;
    _floorLine = nullptr;
    _petImage = nullptr;
    _messImage = nullptr;
    _needsPanel = nullptr;
    for (uint8_t index = 0; index < kNeedCount; ++index) {
        _needLabels[index] = nullptr;
        _needBars[index] = nullptr;
    }
    _footer = nullptr;
    _actionLabel = nullptr;
    _hintLabel = nullptr;
}

void TamagotchiCard::advanceFromMillis(uint32_t nowMillis) {
    const uint32_t elapsedMillis = nowMillis - _lastAdvanceMillis;
    _lastAdvanceMillis = nowMillis;

    const uint64_t elapsedWithRemainder =
        static_cast<uint64_t>(elapsedMillis) + _millisRemainder;
    const uint64_t elapsedSeconds = elapsedWithRemainder / 1000;
    _millisRemainder = static_cast<uint32_t>(elapsedWithRemainder % 1000);
    if (elapsedSeconds == 0) {
        return;
    }

    if (std::numeric_limits<uint64_t>::max() - _sameBootAppliedSeconds < elapsedSeconds) {
        _sameBootAppliedSeconds = std::numeric_limits<uint64_t>::max();
    } else {
        _sameBootAppliedSeconds += elapsedSeconds;
    }

    const bool stageChangeAlreadyPending =
        hasModelChange(_pendingRenderChanges, TamagotchiModelChange::Stage);
    if (!stageChangeAlreadyPending) {
        _stageBeforeLastAdvance = _model.getState().stage;
    }
    recordModelChanges(_model.advanceBy(elapsedSeconds));
}

void TamagotchiCard::tryApplyOfflineCatchUp() {
    if (_catchUpState != OfflineCatchUpState::PendingClock) {
        return;
    }

    const uint32_t nowMillis = millis();
    advanceFromMillis(nowMillis);

    time_t clockEpoch = 0;
    if (!_clockService.tryGetEpoch(clockEpoch) || clockEpoch < 0) {
        return;
    }

    const uint64_t nowEpoch = static_cast<uint64_t>(clockEpoch);
    if (_pendingBaselineEpoch == TamagotchiConstants::NO_CLOCK_EPOCH) {
        recordModelChanges(_model.setLastUpdatedEpoch(nowEpoch));
        _catchUpState = OfflineCatchUpState::RebaselinedWithoutHistory;
    } else if (nowEpoch < _pendingBaselineEpoch) {
        recordModelChanges(_model.setLastUpdatedEpoch(nowEpoch));
        _catchUpState = OfflineCatchUpState::RebaselinedAfterBackwardClock;
    } else {
        const uint64_t rawEpochElapsed = nowEpoch - _pendingBaselineEpoch;
        const uint64_t unappliedSeconds =
            rawEpochElapsed > _sameBootAppliedSeconds
                ? rawEpochElapsed - _sameBootAppliedSeconds
                : 0;
        const uint64_t catchUpSeconds =
            unappliedSeconds > TamagotchiConstants::MAX_OFFLINE_CATCH_UP_SECONDS
                ? TamagotchiConstants::MAX_OFFLINE_CATCH_UP_SECONDS
                : unappliedSeconds;

        if (catchUpSeconds != 0) {
            if (!hasModelChange(_pendingRenderChanges, TamagotchiModelChange::Stage)) {
                _stageBeforeLastAdvance = _model.getState().stage;
            }
            recordModelChanges(_model.advanceBy(catchUpSeconds));
        }
        recordModelChanges(_model.setLastUpdatedEpoch(nowEpoch));
        _catchUpState = OfflineCatchUpState::Applied;
    }

    _timeUnknown = false;
    saveWithClockBaseline(true);
}

bool TamagotchiCard::saveWithClockBaseline(bool force) {
    if (!force && !_persistenceDirty && !_model.isDirty()) {
        return true;
    }

    advanceFromMillis(millis());

    time_t clockEpoch = 0;
    if (_catchUpState == OfflineCatchUpState::PendingClock) {
        if (_clockService.tryGetEpoch(clockEpoch) && clockEpoch >= 0) {
            // A pending session must complete its once-only catch-up before it can
            // save a trusted baseline; otherwise it could replay same-boot time.
            tryApplyOfflineCatchUp();
            return !_persistenceDirty && !_saveFailed;
        }

        if (!_clockService.tryGetEpoch(clockEpoch)) {
            recordModelChanges(_model.setLastUpdatedEpoch(TamagotchiConstants::NO_CLOCK_EPOCH));
        }
    } else if (_clockService.tryGetEpoch(clockEpoch) && clockEpoch >= 0) {
        recordModelChanges(_model.setLastUpdatedEpoch(static_cast<uint64_t>(clockEpoch)));
    }

    if (_stateStore.save(_model.getState())) {
        _model.markPersisted();
        _persistenceDirty = false;
        _saveFailed = false;
        return true;
    }

    _persistenceDirty = true;
    _saveFailed = true;
    return false;
}

void TamagotchiCard::recordModelChanges(TamagotchiModelChange changes) {
    if (changes == TamagotchiModelChange::None) {
        return;
    }

    _pendingRenderChanges |= changes;
    _persistenceDirty = true;
}

void TamagotchiCard::renderModel(uint32_t nowMillis, bool force) {
    if (!isValidObject(_card)) {
        return;
    }

    const TamagotchiState& state = _model.getState();
    const bool initialRender = force || !_rendered.initialized;
    const bool stageChanged = initialRender || _rendered.stage != state.stage ||
                              hasModelChange(_pendingRenderChanges,
                                             TamagotchiModelChange::Stage);

    if (stageChanged && isValidObject(_stageLabel)) {
        lv_label_set_text(_stageLabel, stageText(state.stage));
    }

    for (uint8_t index = 0; index < kNeedCount; ++index) {
        const uint8_t value = needValue(state, index);
        if (initialRender || _rendered.needs[index] != value) {
            updateNeedBar(index, value);
        }
        _rendered.needs[index] = value;
    }

    const bool messChanged = initialRender || _rendered.mess != state.mess ||
                             hasModelChange(_pendingRenderChanges,
                                            TamagotchiModelChange::MessState);
    if (messChanged && isValidObject(_messImage)) {
        if (state.mess) {
            lv_obj_clear_flag(_messImage, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(_messImage, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (initialRender || _rendered.sick != state.sick || _rendered.mess != state.mess ||
        _rendered.timeUnknown != _timeUnknown || _rendered.saveFailed != _saveFailed ||
        hasModelChange(_pendingRenderChanges, TamagotchiModelChange::SickState) ||
        hasModelChange(_pendingRenderChanges, TamagotchiModelChange::MessState)) {
        renderStatus(state);
    }

    const bool evolvedFromChild = hasModelChange(_pendingRenderChanges,
                                                  TamagotchiModelChange::Stage) &&
                                  _stageBeforeLastAdvance == TamagotchiStage::Child &&
                                  state.stage == TamagotchiStage::Adult;
    if (evolvedFromChild) {
        startEvolutionVisual(nowMillis);
    } else if (_oneShotVisual == PetVisual::None) {
        setVisual(steadyVisual(), nowMillis);
    }

    _rendered.stage = state.stage;
    _rendered.sick = state.sick;
    _rendered.mess = state.mess;
    _rendered.timeUnknown = _timeUnknown;
    _rendered.saveFailed = _saveFailed;
    _rendered.initialized = true;
    _pendingRenderChanges = TamagotchiModelChange::None;
}

void TamagotchiCard::renderStatus(const TamagotchiState& state) {
    if (!isValidObject(_statusLabel)) {
        return;
    }

    const char* text = "";
    lv_color_t color = Style::labelColor();
    if (_saveFailed) {
        text = "SAVE!";
        color = lv_color_hex(kErrorColor);
    } else if (_timeUnknown) {
        text = "TIME?";
        color = lv_color_hex(kWarningColor);
    } else if (state.sick) {
        text = "SICK";
        color = lv_color_hex(kErrorColor);
    } else if (state.mess) {
        text = "MESS";
        color = lv_color_hex(kMessColor);
    }

    lv_label_set_text(_statusLabel, text);
    lv_obj_set_style_text_color(_statusLabel, color, 0);
}

void TamagotchiCard::updateNeedBar(uint8_t index, uint8_t value) {
    if (index >= kNeedCount || !isValidObject(_needBars[index])) {
        return;
    }

    lv_color_t color = Style::accentColor();
    if (value < 30) {
        color = lv_color_hex(kErrorColor);
    } else if (value < 60) {
        color = lv_color_hex(kWarningColor);
    }

    lv_bar_set_value(_needBars[index], value, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(_needBars[index], color, LV_PART_INDICATOR);
}

void TamagotchiCard::expireTimedUi(uint32_t nowMillis) {
    if (_oneShotVisual != PetVisual::None &&
        deadlineReached(nowMillis, _oneShotDeadlineMillis)) {
        _oneShotVisual = PetVisual::None;
        _oneShotDeadlineMillis = 0;
        setVisual(steadyVisual(), nowMillis);
    }

    if (_resultDeadlineMillis != 0 && deadlineReached(nowMillis, _resultDeadlineMillis)) {
        _resultDeadlineMillis = 0;
        _resultText[0] = '\0';
        if (isValidObject(_resultLabel)) {
            lv_label_set_text(_resultLabel, "");
        }
    }
}

TamagotchiCard::PetVisual TamagotchiCard::steadyVisual() const {
    const TamagotchiState& state = _model.getState();
    switch (state.stage) {
    case TamagotchiStage::Egg:
        return PetVisual::Egg;
    case TamagotchiStage::Child:
        return state.sick ? PetVisual::ChildSick : PetVisual::ChildIdle;
    case TamagotchiStage::Adult:
        return state.sick ? PetVisual::AdultSick : PetVisual::AdultIdle;
    default:
        return PetVisual::None;
    }
}

void TamagotchiCard::setVisual(PetVisual requested, uint32_t nowMillis) {
    if (requested == _currentVisual) {
        return;
    }

    if (requested == PetVisual::None) {
        if (isValidObject(_petImage)) {
            stopPetAnimation(_petImage);
        }
        _currentVisual = PetVisual::None;
        return;
    }

    const lv_img_dsc_t** frames = nullptr;
    uint8_t frameCount = 0;
    uint32_t durationMs = 0;
    bool loop = false;

    switch (requested) {
    case PetVisual::Egg:
        frames = tamagotchi_egg_sprites;
        frameCount = tamagotchi_egg_sprites_count;
        durationMs = kEggDurationMs;
        loop = true;
        break;
    case PetVisual::ChildIdle:
        frames = tamagotchi_child_idle_sprites;
        frameCount = tamagotchi_child_idle_sprites_count;
        durationMs = kIdleDurationMs;
        loop = true;
        break;
    case PetVisual::AdultIdle:
        frames = tamagotchi_adult_idle_sprites;
        frameCount = tamagotchi_adult_idle_sprites_count;
        durationMs = kIdleDurationMs;
        loop = true;
        break;
    case PetVisual::ChildSick:
        frames = tamagotchi_child_sick_sprites;
        frameCount = tamagotchi_child_sick_sprites_count;
        durationMs = kSickDurationMs;
        loop = true;
        break;
    case PetVisual::AdultSick:
        frames = tamagotchi_adult_sick_sprites;
        frameCount = tamagotchi_adult_sick_sprites_count;
        durationMs = kSickDurationMs;
        loop = true;
        break;
    case PetVisual::ChildFeed:
        frames = tamagotchi_child_feed_sprites;
        frameCount = tamagotchi_child_feed_sprites_count;
        durationMs = kActionDurationMs;
        break;
    case PetVisual::AdultFeed:
        frames = tamagotchi_adult_feed_sprites;
        frameCount = tamagotchi_adult_feed_sprites_count;
        durationMs = kActionDurationMs;
        break;
    case PetVisual::ChildPlay:
        frames = tamagotchi_child_play_sprites;
        frameCount = tamagotchi_child_play_sprites_count;
        durationMs = kActionDurationMs;
        break;
    case PetVisual::AdultPlay:
        frames = tamagotchi_adult_play_sprites;
        frameCount = tamagotchi_adult_play_sprites_count;
        durationMs = kActionDurationMs;
        break;
    case PetVisual::ChildClean:
        frames = tamagotchi_child_clean_sprites;
        frameCount = tamagotchi_child_clean_sprites_count;
        durationMs = kActionDurationMs;
        break;
    case PetVisual::AdultClean:
        frames = tamagotchi_adult_clean_sprites;
        frameCount = tamagotchi_adult_clean_sprites_count;
        durationMs = kActionDurationMs;
        break;
    case PetVisual::ChildRest:
        frames = tamagotchi_child_rest_sprites;
        frameCount = tamagotchi_child_rest_sprites_count;
        durationMs = kRestDurationMs;
        break;
    case PetVisual::AdultRest:
        frames = tamagotchi_adult_rest_sprites;
        frameCount = tamagotchi_adult_rest_sprites_count;
        durationMs = kRestDurationMs;
        break;
    case PetVisual::ChildDoctor:
        frames = tamagotchi_child_doctor_sprites;
        frameCount = tamagotchi_child_doctor_sprites_count;
        durationMs = kDoctorDurationMs;
        break;
    case PetVisual::AdultDoctor:
        frames = tamagotchi_adult_doctor_sprites;
        frameCount = tamagotchi_adult_doctor_sprites_count;
        durationMs = kDoctorDurationMs;
        break;
    case PetVisual::Evolve:
        frames = tamagotchi_evolve_sprites;
        frameCount = tamagotchi_evolve_sprites_count;
        durationMs = kEvolveDurationMs;
        break;
    case PetVisual::None:
    default:
        break;
    }

    if (!setPetAnimation(frames, frameCount, durationMs, loop)) {
        if (!_visualFaultLogged) {
            Serial.println("Tamagotchi visual group invalid");
            _visualFaultLogged = true;
        }
        return;
    }

    _currentVisual = requested;
    if (!loop) {
        _oneShotVisual = requested;
        _oneShotDeadlineMillis = nowMillis + durationMs;
    }
}

bool TamagotchiCard::setPetAnimation(const lv_img_dsc_t* frames[], uint8_t frameCount,
                                     uint32_t durationMs, bool loop) {
    if (!isValidObject(_petImage) || frames == nullptr || frameCount == 0 || durationMs == 0) {
        return false;
    }

    for (uint8_t index = 0; index < frameCount; ++index) {
        if (frames[index] == nullptr) {
            return false;
        }
    }

    stopPetAnimation(_petImage);
    lv_animimg_set_src(_petImage, (const void**)frames, frameCount);
    lv_image_set_src(_petImage, frames[0]);
    lv_animimg_set_duration(_petImage, durationMs);
    lv_animimg_set_repeat_count(_petImage, loop ? LV_ANIM_REPEAT_INFINITE : 0);
    lv_animimg_start(_petImage);
    return true;
}

bool TamagotchiCard::stopPetAnimation(lv_obj_t* petImage) {
    if (!isValidObject(petImage)) {
        return false;
    }

    // LVGL 9.2.2 exposes no lv_animimg_delete(). Its animimage implementation
    // stores its sole internal animation with the animimage object as var, so this
    // is the equivalent narrowly scoped LVGL 9.2 stop operation.
    return lv_anim_delete(petImage, nullptr);
}

void TamagotchiCard::startEvolutionVisual(uint32_t nowMillis) {
    _oneShotVisual = PetVisual::None;
    _oneShotDeadlineMillis = 0;
    setVisual(PetVisual::Evolve, nowMillis);
    if (_currentVisual != PetVisual::Evolve) {
        setVisual(steadyVisual(), nowMillis);
    }
}

void TamagotchiCard::startActionVisual(TamagotchiAction action, uint32_t nowMillis) {
    const TamagotchiStage stage = _model.getState().stage;
    if (stage == TamagotchiStage::Egg) {
        return;
    }

    PetVisual visual = PetVisual::None;
    if (stage == TamagotchiStage::Child) {
        switch (action) {
        case TamagotchiAction::Feed:
            visual = PetVisual::ChildFeed;
            break;
        case TamagotchiAction::Play:
            visual = PetVisual::ChildPlay;
            break;
        case TamagotchiAction::Clean:
            visual = PetVisual::ChildClean;
            break;
        case TamagotchiAction::Rest:
            visual = PetVisual::ChildRest;
            break;
        case TamagotchiAction::Doctor:
            visual = PetVisual::ChildDoctor;
            break;
        case TamagotchiAction::Count:
        default:
            break;
        }
    } else if (stage == TamagotchiStage::Adult) {
        switch (action) {
        case TamagotchiAction::Feed:
            visual = PetVisual::AdultFeed;
            break;
        case TamagotchiAction::Play:
            visual = PetVisual::AdultPlay;
            break;
        case TamagotchiAction::Clean:
            visual = PetVisual::AdultClean;
            break;
        case TamagotchiAction::Rest:
            visual = PetVisual::AdultRest;
            break;
        case TamagotchiAction::Doctor:
            visual = PetVisual::AdultDoctor;
            break;
        case TamagotchiAction::Count:
        default:
            break;
        }
    }

    if (visual != PetVisual::None) {
        if (_oneShotVisual == visual && _currentVisual == visual) {
            // A later applied action replaces the earlier one-shot. Preserve the
            // no-restart gate in setVisual() for all steady-state rendering.
            _currentVisual = PetVisual::None;
        }
        setVisual(visual, nowMillis);
    }
}

void TamagotchiCard::setTransientResult(TamagotchiAction action,
                                        TamagotchiActionResult result,
                                        uint32_t nowMillis) {
    const char* text = "";
    uint32_t durationMs = 0;

    if (result == TamagotchiActionResult::Applied) {
        switch (action) {
        case TamagotchiAction::Feed:
            text = "FED";
            durationMs = kResultDurationMs;
            break;
        case TamagotchiAction::Play:
            text = "PLAYED";
            durationMs = kResultDurationMs;
            break;
        case TamagotchiAction::Clean:
            text = "CLEAN";
            durationMs = kResultDurationMs;
            break;
        case TamagotchiAction::Rest:
            text = "RESTED";
            durationMs = kResultDurationMs;
            break;
        case TamagotchiAction::Doctor:
            text = "BETTER";
            durationMs = kRefusalResultDurationMs;
            break;
        case TamagotchiAction::Count:
        default:
            text = "ACTION?";
            durationMs = kRefusalResultDurationMs;
            break;
        }
    } else if (result == TamagotchiActionResult::RefusedLowEnergy) {
        text = "TOO TIRED";
        durationMs = kRefusalResultDurationMs;
    } else if (result == TamagotchiActionResult::RefusedNotSick) {
        text = "NOT SICK";
        durationMs = kRefusalResultDurationMs;
    } else if (result == TamagotchiActionResult::RefusedWhileEgg) {
        text = "HATCH FIRST";
        durationMs = kRefusalResultDurationMs;
    } else {
        text = "ACTION?";
        durationMs = kRefusalResultDurationMs;
    }

    std::snprintf(_resultText, sizeof(_resultText), "%s", text);
    _resultDeadlineMillis = nowMillis + durationMs;
    if (isValidObject(_resultLabel)) {
        lv_label_set_text(_resultLabel, _resultText);
    }
}

void TamagotchiCard::renderFooter(bool selectorOpen, TamagotchiAction selectedAction) {
    if (!isValidObject(_actionLabel) || !isValidObject(_hintLabel)) {
        return;
    }

    if (selectorOpen) {
        lv_label_set_text(_actionLabel, actionText(selectedAction));
        lv_label_set_text(_hintLabel, "UP/DN  OK");
        return;
    }

    lv_label_set_text(_actionLabel, "TAMAGOTCHI");
    lv_label_set_text(_hintLabel, "UP/DN NAV");
}
