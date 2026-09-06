# Input and navigation

DeskHog has three buttons managed by Bounce2. The LVGL handler task polls them and sends press events to `CardNavigationStack`, which gives the active card first refusal before applying default navigation.

## Button mapping

| Logical button | Default action |
|---|---|
| `BUTTON_DOWN` | Next card |
| `BUTTON_CENTER` | Card-specific action |
| `BUTTON_UP` | Previous card |

The constants currently serve as both indexes into the global `buttons[]` array and GPIO numbers. Use the names from `Input`; the authoritative GPIO and electrical-level table is in [Hardware and power](hardware-and-power.md#buttons).

`Input::configureButtons()` attaches each button, sets a 5 ms debounce interval, and configures its pressed state.

## Polling flow

`lvglTask` runs approximately every 5 ms, but it updates all Bounce2 instances every 50 ms. On each poll:

1. Update all three Bounce2 objects.
2. Check the center+down deep-sleep chord.
3. If the chord is not active, forward each new `.pressed()` edge to `CardNavigationStack::handleButtonPress()`.

Only press edges are forwarded through `InputHandler`. Cards that need held/released state, such as Paddle, inspect the shared Bounce2 button objects from their UI-task `update()` method.

## Dispatch rules

For the active card, `CardNavigationStack` looks up its registered `InputHandler` and calls:

```cpp
bool handled = handler->handleButtonPress(buttonIndex);
```

- Return `true` to consume the event and suppress navigation.
- Return `false` to allow default handling.
- Default handling maps down to `nextCard()` and up to `prevCard()`.
- Center has no default action.
- Cards without a registered handler always receive default navigation.

This supports a useful pattern: consume up/down while a game is active, but return `false` on its start or game-over screen so the user can leave the card.

## Navigation stack

`CardNavigationStack` uses a vertically scrolling LVGL flex container with snap-to-center behavior. A 2-pixel indicator strip on the right contains one pip per card.

- `nextCard()` and `prevCard()` wrap at the ends.
- `goToCard()` animates the container over 200 ms.
- Adding/removing cards recreates the indicator pips.
- Removing the active card selects a valid neighboring card.
- The provisioning card remains the first child.

The current index is managed explicitly by button navigation. The LVGL scroll callback does not derive a new index from arbitrary manual scrolling.

## Active-card updates

After draining UI callbacks, `CardController::processUIQueue()` calls `CardNavigationStack::updateActiveCard()`. Only the visible card's handler receives `update()`.

Although `InputHandler::update()` returns `bool`, the current caller ignores the return value. Returning `false` does not disable future updates.

## Recommended input patterns

### Simple center action

```cpp
bool MyCard::handleButtonPress(uint8_t buttonIndex) {
    if (buttonIndex != Input::BUTTON_CENTER) return false;
    advanceState();
    return true;
}
```

### Game with held controls

- Use `handleButtonPress()` to declare which buttons the game consumes in each state.
- Read `.isPressed()`/`.released()` from the shared Bounce2 objects in `update()`.
- Stop movement on pause, game over, and card removal.
- Permit up/down navigation from at least one non-playing state.

Do not create a separate input task for a card; button state and active-card updates already execute in the LVGL handler context.

## Sleep chord

Holding center and down for two seconds calls `esp_deep_sleep_start()`. Individual press behavior is suppressed while the chord is active. The code disables deep-sleep GPIO wake immediately before sleeping, so the documented wake path is the hardware reset control.

GPIO wake sources are also configured for automatic light sleep during normal operation. See [Hardware and power](hardware-and-power.md).

## Current caveats

- `CardNavigationStack` supports a mutex pointer, but normal initialization calls `setDisplayInterface()` before the stack exists, so no mutex is assigned there today.
- Flappy and Paddle read edge state again inside `update()`. Because Bounce2 is refreshed every 50 ms while card updates run more often, an edge can remain observable for multiple update iterations. New games should make their edge/held semantics explicit.
