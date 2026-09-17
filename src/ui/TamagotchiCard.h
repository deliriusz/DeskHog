#pragma once

#include <cstdint>

#include <lvgl.h>

#include "tamagotchi/TamagotchiModel.h"
#include "tamagotchi/TamagotchiStateStore.h"
#include "time/ClockService.h"
#include "ui/InputHandler.h"

class TamagotchiCard : public InputHandler {
public:
    TamagotchiCard(lv_obj_t* parent, TamagotchiStateStore& stateStore,
                   ClockService& clockService);
    ~TamagotchiCard() override;

    lv_obj_t* getCard() const;
    bool handleButtonPress(uint8_t buttonIndex) override;
    bool update() override;
    void prepareForRemoval() override;

private:
    enum class TamagotchiInteractionMode : uint8_t {
        Egg,
        Normal,
        ActionSelector
    };

    enum class OfflineCatchUpState : uint8_t {
        PendingClock,
        Applied,
        RebaselinedWithoutHistory,
        RebaselinedAfterBackwardClock
    };

    enum class PetVisual : uint8_t {
        None,
        Egg,
        ChildIdle,
        AdultIdle,
        ChildSick,
        AdultSick,
        ChildFeed,
        AdultFeed,
        ChildPlay,
        AdultPlay,
        ChildClean,
        AdultClean,
        ChildRest,
        AdultRest,
        ChildDoctor,
        AdultDoctor,
        Evolve
    };

    struct InitialSession {
        TamagotchiState state{};
        bool persistenceDirty = false;
        bool saveFailed = false;
    };

    struct RenderSnapshot {
        TamagotchiStage stage = TamagotchiStage::Egg;
        uint8_t needs[5] = {0, 0, 0, 0, 0};
        bool sick = false;
        bool mess = false;
        bool timeUnknown = true;
        bool saveFailed = false;
        bool initialized = false;
    };

    static InitialSession loadInitialSession(TamagotchiStateStore& stateStore);

    TamagotchiCard(lv_obj_t* parent, TamagotchiStateStore& stateStore,
                   ClockService& clockService, InitialSession initialSession);

    static bool deadlineReached(uint32_t nowMillis, uint32_t deadlineMillis);
    static bool isValidObject(const lv_obj_t* object);

    bool createUi(lv_obj_t* parent);
    void destroyUnownedUi();
    void clearUiPointers();

    void advanceFromMillis(uint32_t nowMillis);
    void tryApplyOfflineCatchUp();
    bool saveWithClockBaseline(bool force);
    void recordModelChanges(TamagotchiModelChange changes);

    void renderModel(uint32_t nowMillis, bool force);
    void renderStatus(const TamagotchiState& state);
    void updateNeedBar(uint8_t index, uint8_t value);
    void expireTimedUi(uint32_t nowMillis);
    void expireTransientResult(uint32_t nowMillis);

    PetVisual steadyVisual() const;
    void setVisual(PetVisual requested, uint32_t nowMillis);
    bool setPetAnimation(const lv_img_dsc_t* frames[], uint8_t frameCount,
                         uint32_t durationMs, bool loop);
    static bool stopPetAnimation(lv_obj_t* petImage);
    void startEvolutionVisual(uint32_t nowMillis);

    void startActionVisual(TamagotchiAction action, uint32_t nowMillis);
    void setInteractionMode(TamagotchiInteractionMode mode);
    void moveSelection(int8_t direction);
    void executeSelectedAction(uint32_t nowMillis);
    void requestImmediateSave();
    void setTransientResult(TamagotchiAction action, TamagotchiActionResult result,
                            uint32_t nowMillis);
    void setTransientResultText(const char* text, uint32_t durationMillis,
                                uint32_t nowMillis);
    void renderInteractionUi();
    void renderFooter(bool selectorOpen, TamagotchiAction selectedAction);

    TamagotchiStateStore& _stateStore;
    ClockService& _clockService;
    TamagotchiModel _model;
    bool _persistenceDirty;
    bool _saveFailed;

    uint32_t _lastAdvanceMillis;
    uint32_t _millisRemainder;
    uint64_t _sameBootAppliedSeconds;
    uint64_t _pendingBaselineEpoch;
    OfflineCatchUpState _catchUpState;
    bool _timeUnknown;

    uint32_t _lastModelUpdateMillis;
    TamagotchiModelChange _pendingRenderChanges;
    TamagotchiStage _stageBeforeLastAdvance;

    TamagotchiInteractionMode _interactionMode;
    TamagotchiAction _selectedAction;
    bool _immediateSaveRequested;

    PetVisual _currentVisual;
    PetVisual _oneShotVisual;
    uint32_t _oneShotDeadlineMillis;
    uint32_t _resultDeadlineMillis;
    char _resultText[24];
    RenderSnapshot _rendered;
    bool _preparedForRemoval;
    bool _visualFaultLogged;

    lv_obj_t* _card = nullptr;
    lv_obj_t* _header = nullptr;
    lv_obj_t* _stageLabel = nullptr;
    lv_obj_t* _resultLabel = nullptr;
    lv_obj_t* _statusLabel = nullptr;
    lv_obj_t* _roomViewport = nullptr;
    lv_obj_t* _floorLine = nullptr;
    lv_obj_t* _petImage = nullptr;
    lv_obj_t* _messImage = nullptr;
    lv_obj_t* _needsPanel = nullptr;
    lv_obj_t* _needLabels[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    lv_obj_t* _needBars[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    lv_obj_t* _footer = nullptr;
    lv_obj_t* _actionLabel = nullptr;
    lv_obj_t* _hintLabel = nullptr;
};
