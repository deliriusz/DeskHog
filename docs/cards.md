# Cards

Cards are full-screen LVGL views arranged vertically by `CardNavigationStack`. `CardController` owns the catalog of available card types, creates configured instances, registers input handlers, and rebuilds the stack after configuration changes.

## Built-in card types

| `CardType` | Class | Multiple instances | Configuration |
|---|---|---:|---|
| `INSIGHT` | `InsightCard` | Yes | PostHog insight short ID |
| `FRIEND` | `FriendCard` | No | None |
| `HELLO_WORLD` | `HelloWorldCard` | Yes | None |
| `FLAPPY_HOG` | `FlappyHogCard` | No | None |
| `QUESTION` | `QuestionCard` | No | None |
| `PADDLE` | `PaddleCard` | No | None |
| `TAMAGOTCHI` | `TamagotchiCard` | No | None; state is separate |

`ProvisioningCard` is not configurable. It is created first, kept at stack index 0, and cannot be removed through the portal.

## Configuration types

`src/config/CardConfig.h` defines three pieces of the card system:

- `CardType`: stable firmware identifier for a kind of card.
- `CardDefinition`: catalog metadata and a factory function used by firmware and exposed to the portal.
- `CardConfig`: one persisted card instance with `type`, `config`, `order`, and `name`.

Every `CardType` must be handled by `cardTypeToString()` and `tryStringToCardType()`. These strings are persisted in NVS and exchanged with the portal, so changing one is a storage/API migration. `tryStringToCardType()` accepts only exact stable spellings and leaves its output unchanged on failure. The older `stringToCardType()` fallback remains only for source compatibility; HTTP and NVS ingestion must use the non-fallback API.

## Creation and reconciliation

During `CardController::initialize()`:

1. The display reference and UI callback queue are initialized.
2. Built-in `CardDefinition` factories are registered.
3. `CardNavigationStack` and the permanent provisioning card are created.
4. Stored `CardConfig` values are loaded.
5. Configured cards are stably sorted by `order`, defensively filtered through the registered definitions, and created through their factories.

After `CARD_CONFIG_CHANGED`, the controller dispatches reconciliation to the UI queue. Current reconciliation is a full rebuild, not a fine-grained diff:

1. Save the visible card index.
2. Call `prepareForRemoval()` on every dynamic card.
3. Ask the navigation stack to remove and delete each LVGL root.
4. Delete each C++ card handler and clear tracking.
5. Stably sort the new configuration, retain only the first ordered occurrence of every singleton definition, and recreate the effective list.
6. Rebuild navigation indicators and select the new or previous position.

Factories add `CardInstance { handler, lvglCard }` to `dynamicCards`, register the handler with the navigation stack, and return the LVGL root for insertion.

`allowMultiple` is enforced at both persistence boundaries. The portal rejects a duplicate of any singleton definition before it writes NVS. Runtime reconciliation repeats the policy for crafted or legacy NVS: it creates only the first stable-ordered singleton occurrence, logs the skipped type/position, and never rewrites the stored list. Repeatable definitions remain repeatable.

`TAMAGOTCHI` is a singleton no-config card. Its factory borrows the boot-lifetime `TamagotchiStateStore` and `ClockService`; removing the card changes only the `cards` list and never deletes `tamagotchi/state`.

## Minimal card contract

Every configurable card implements `InputHandler` and exposes its LVGL root. The controller stores each dynamic instance as an `InputHandler*` and calls `prepareForRemoval()` during reconciliation, so this inheritance is required even when the default input and update methods are sufficient.

```cpp
class WeatherCard : public InputHandler {
public:
    explicit WeatherCard(lv_obj_t* parent);
    ~WeatherCard() override;

    lv_obj_t* getCard() const { return _card; }
    bool handleButtonPress(uint8_t buttonIndex) override;
    void prepareForRemoval() override { _card = nullptr; }

private:
    lv_obj_t* _card = nullptr;
    lv_obj_t* _label = nullptr;
};
```

The constructor should create the root and children, apply explicit sizing/styles, and leave the root valid for `CardNavigationStack::addCard()`.

`handleButtonPress()` returns `true` when the card consumes the button. Returning `false` allows the navigation stack to apply its default up/down paging behavior. See [Input and navigation](input-and-navigation.md).

## Adding a card type

1. Add the value to `CardType` in `src/config/CardConfig.h`.
2. Add a matching `cardTypeToString()` case and exact `tryStringToCardType()` branch.
3. Create focused `.h` and `.cpp` files, normally under `src/ui/`.
4. Inherit from `InputHandler`; override button or update methods only as needed.
5. Include the card in `src/ui/CardController.h`.
6. Register a `CardDefinition` and factory in `CardController::initializeCardTypes()`.
7. In the factory, allocate the card, validate `getCard()`, track it in `dynamicCards`, register its input handler, and return its LVGL root.
8. Build and verify add, remove, reorder, navigation, and cleanup on hardware.

Factory pattern:

```cpp
CardDefinition weatherDef;
weatherDef.type = CardType::WEATHER;
weatherDef.name = "Weather";
weatherDef.allowMultiple = false;
weatherDef.needsConfigInput = true;
weatherDef.configInputLabel = "Location";
weatherDef.uiDescription = "Shows the current weather";
weatherDef.factory = [this](const String& configValue) -> lv_obj_t* {
    WeatherCard* card = new WeatherCard(screen);
    if (card && card->getCard()) {
        dynamicCards[CardType::WEATHER].push_back({card, card->getCard()});
        cardStack->registerInputHandler(card->getCard(), card);
        return card->getCard();
    }
    delete card;
    return nullptr;
};
registerCardType(weatherDef);
```

The portal builds its add-card UI from `GET /api/cards/definitions`; no card-specific portal UI is needed when a single string configuration is sufficient.

## Porting a legacy or hackathon card

Port old implementations into the card architecture instead of preserving direct hooks in core systems. First inventory:

- LVGL objects and who owns/deletes them;
- direct references from `src/main.cpp` or `CardController`;
- custom button polling or handling;
- any loop previously called directly by the LVGL task;
- state that must survive within the card wrapper or a separate model.

Then:

1. Wrap the existing feature in a card class that implements `InputHandler`.
2. Expose one LVGL root through `getCard()`.
3. Route edge-triggered controls through `handleButtonPress()` and return `false` only when default card navigation should continue.
4. Move continuous work into `update()`; `CardNavigationStack::updateActiveCard()` invokes it from `CardController::processUIQueue()` while the card is visible.
5. Implement `prepareForRemoval()` so stack-owned LVGL deletion cannot be repeated by the wrapper or engine.
6. Add the `CardType` and both string conversions, then register the `CardDefinition` and factory as described above.
7. Remove former construction, loop, input, and card-specific controller hooks from core code.

Use [Games](games.md) for game-specific model, input, timing, and cleanup guidance. A port is complete when:

- the definition appears in the portal's add-card UI;
- the card can be added, removed, reordered, and revisited dynamically;
- controls and continuous updates work without a card-specific path in `src/main.cpp`;
- repeated removal does not leak memory, retain invalid callbacks, or double-delete LVGL objects;
- the device remains stable while network and other background work runs.

## Showing data

Create local LVGL state on the UI task. For background data, follow [Display and LVGL](display-and-lvgl.md#displaying-background-data); for PostHog renderers, use [PostHog insights](posthog-insights.md); for continuously changing state, use [Games](games.md).

## Cleanup and ownership

`CardNavigationStack::removeCard()` deletes the LVGL root. It first removes the root/handler association, then calls `lv_obj_del()` and refreshes the stack.

The controller calls `prepareForRemoval()` before this deletion. Use it to mark the root as externally owned or null the pointer so the C++ destructor does not delete it again. Parent deletion already deletes all LVGL children.

Queued callbacks and event subscriptions may outlive a card. The current `EventQueue` has no unsubscribe API, so avoid adding a dynamically removable subscriber without first designing safe lifetime management.

## Current caveats

- Unknown or malformed persisted entries are skipped, never converted to `INSIGHT`, and are not repaired behind the user's back.
- Reconciliation remains a full configurable-card rebuild, not an in-place diff.
- Saving a newly fetched insight title republishes `CARD_CONFIG_CHANGED`, which can cause a card-stack rebuild.
