# Games

A game is an interactive card whose `InputHandler::update()` advances state and renders while that card is visible. It should use the existing LVGL task instead of creating its own render or input task.

## Recommended structure

Separate the game into two responsibilities:

- **Game model/engine:** state machine, positions, velocities, collisions, scores, and reset logic.
- **Card wrapper/view:** LVGL objects, button-consumption policy, reading held input, rendering model state, and card lifecycle.

`PaddleGame` plus `PaddleCard` follows this split. `FlappyHogCard` wraps `FlappyBirdGame`, whose engine also owns its LVGL objects. Model/view separation makes non-visual game logic easier to reason about and eventually test.

## Lifecycle

1. The card factory allocates the wrapper.
2. The wrapper creates its root LVGL object and game elements.
3. The factory tracks the wrapper and registers it with `CardNavigationStack`.
4. While visible, `updateActiveCard()` calls its `update()` on the LVGL task.
5. `handleButtonPress()` decides whether each press is consumed or becomes card navigation.
6. Before removal, `prepareForRemoval()` prevents double deletion.
7. The stack deletes the LVGL root; the wrapper destructor must release non-LVGL game resources.

The current Flappy wrapper violates the last requirement: after `prepareForRemoval()` sets `markedForRemoval`, its destructor skips `game->cleanup()` and `delete game`. Normal reconciliation therefore appears to leak `FlappyBirdGame`. Do not copy this pattern.

## Update loop

A typical update performs four stages:

```cpp
bool GameCard::update() {
    readHeldInput();
    game.update(millis());
    renderGameState();
    renderMessagesAndScore();
    return true;
}
```

Only the active card is updated. `lvglTask` has a 5 ms outer delay, not a guaranteed frame rate. Use elapsed time for speed-sensitive behavior; current Paddle and Flappy movement remains frame-rate-dependent.

The current caller ignores the Boolean return from `update()`, so it cannot be used to unsubscribe a game from updates.

## Input policy

Decide controls per state:

| State | Center | Up/down |
|---|---|---|
| Start screen | Start/restart | Usually allow navigation |
| Playing | Primary action or pause | Consume if used by gameplay |
| Paused | Resume | Decide whether navigation is allowed |
| Game over | Restart | Allow navigation so the card is escapable |

For edge-triggered actions, `handleButtonPress()` can update the model immediately. For held movement, inspect the shared Bounce2 state from `update()` and have `handleButtonPress()` return `true` so navigation does not also occur.

## Rendering

- Create LVGL objects once where possible and update their positions, labels, colors, and visibility.
- Keep rendering on the active card's `update()` path or dispatch it through the UI callback queue.
- Avoid allocation in every frame.
- Use `lv_obj_is_valid()` before delayed or conditional updates.
- Keep score buffers large enough for their maximum value.
- Hide state-specific messages instead of repeatedly creating and deleting them.

## State and physics

- Use an explicit enum for states such as start, serve delay, playing, paused, and game over.
- Keep reset behavior deterministic: reset positions, velocities, scores, timers, and input flags together.
- Base timed transitions on `millis()` deltas.
- Keep play-area dimensions consistent between the model and the LVGL view.
- Seed randomness once, not once per frame.

## Adding a game

Follow the registration and ownership contract in [Cards](cards.md). Game-specific work is to:

1. separate model and view where practical;
2. define edge and held-input behavior using [Input and navigation](input-and-navigation.md);
3. make speed-sensitive state use elapsed time;
4. ensure `prepareForRemoval()` and the destructor divide LVGL and non-LVGL ownership without leaks or double deletion;
5. exercise every state transition, navigation away, repeated removal, and concurrent network activity on hardware.

## Existing games

### Paddle

`PaddleGame` owns gameplay state and physics. `PaddleCard` owns LVGL elements, reads held up/down input, maps center to start/pause/resume/restart, renders positions and scores, and allows paging from start/game-over states.

### Flappy Hog

`FlappyHogCard` delegates its active update to `FlappyBirdGame::loop()`. The engine owns LVGL creation, reads center-button edges, applies gravity/collision/pipe movement, renders positions, and resets after game over. The wrapper returns `false` for button presses, so up/down remain navigation controls.

## Validation checklist

- Start, pause, resume, scoring, collision, win/loss, restart, and navigation work.
- Holding a button does not accidentally page the card.
- Leaving the card stops gameplay updates because only the active handler is called.
- Removing/re-adding the card does not double-delete or retain invalid pointers.
- Game speed remains stable during network requests and portal actions.
- No game code calls LVGL from a separate task.
