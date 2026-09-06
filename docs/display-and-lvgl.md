# Display and LVGL

DeskHog renders a 240×135 landscape UI with LVGL 9.2.2 and an ST7789 TFT connected over SPI. `DisplayInterface` owns the driver, buffers, LVGL display registration, flush callback, backlight, and display mutex.

## Initialization

`DisplayInterface`:

1. Creates the `Adafruit_ST7789` driver.
2. Allocates two `lv_color_t` buffers sized for 240×135 pixels each.
3. Creates the LVGL mutex.
4. Starts SPI and initializes the TFT as 135×240 with rotation 1.
5. Configures 5 kHz, 8-bit PWM backlight control and starts at 204/255 brightness.
6. Calls `lv_init()` and creates a 240×135 LVGL display.
7. Registers `_disp_flush()` and the two buffers in partial-render mode.
8. Sets the active screen background to black.

The flush callback selects the TFT address window, writes the rendered pixels, and calls `lv_display_flush_ready()`.

## LVGL tasks

- `lv_tick_task` increments LVGL's clock by 10 ms every 10 ms.
- `lvglTask` calls `DisplayInterface::handleLVGLTasks()`, which holds the display mutex around `lv_timer_handler()`.
- The same task then drains `CardController`'s UI callback queue and updates the active card.

The display mutex prevents concurrent protected sections, but the architectural rule is stronger: background tasks should marshal LVGL changes to `lvglTask` rather than treating the mutex as permission to render from any task.

## UI callback queue

`CardController` creates a FreeRTOS queue with capacity 20 containing `UICallback*`. `dispatchToLVGLTask()` (also exposed through `globalUIDispatch`) allocates a callback and sends it to the back or front of the queue without waiting. `processUIQueue()` executes and deletes each callback on `lvglTask`.

Queue submission can fail. When it does, the callback is deleted and the visual update is discarded. Coalesce high-frequency updates and avoid capturing pointers whose lifetime may end before execution.

## Displaying local data

For data already owned by the active UI task, create LVGL objects in the card constructor and update them directly in `InputHandler::update()` or `handleButtonPress()`:

```cpp
_valueLabel = lv_label_create(_card);
lv_obj_set_style_text_font(_valueLabel, Style::largeValueFont(), 0);
lv_obj_set_style_text_color(_valueLabel, Style::valueColor(), 0);
lv_obj_center(_valueLabel);

// Later, while executing on the UI task:
lv_label_set_text(_valueLabel, formattedValue);
```

Use `Style::labelFont()`, `Style::valueFont()`, `Style::largeValueFont()`, and the shared color helpers for visual consistency.

## Displaying background data

Background producers should publish a domain event or otherwise hand off an owned data copy. Subscription syntax and delivery semantics belong to [Event queue](event-queue.md); inside the short consumer callback, copy the required data and schedule only the LVGL portion:

```cpp
String text = event.jsonData;
globalUIDispatch([this, text]() {
    if (_label && lv_obj_is_valid(_label)) {
        lv_label_set_text(_label, text.c_str());
    }
}, false);
```

For a complete network-to-render example, see [PostHog insights](posthog-insights.md).

## Layout guidance

- The navigation stack reserves 7 horizontal pixels; each card is resized to `screenWidth - 7` by the full screen height.
- Disable scrolling on card roots unless scrolling is an intentional part of the card.
- Set explicit background, border, padding, and radius values; LVGL defaults can consume scarce space unexpectedly.
- With flex or percentage layouts, call `lv_obj_update_layout()` or allow a refresh before relying on calculated dimensions.
- Insight renderers deliberately invalidate and refresh their container between element creation and data layout.

## Object lifecycle

- Check `lv_obj_is_valid()` before delayed updates.
- Parent deletion deletes children; avoid deleting the same LVGL object from both a card destructor and `CardNavigationStack`.
- Implement `prepareForRemoval()` when the stack will delete the root object.
- Capture immutable data rather than temporary references in queued callbacks.
- A queued callback capturing `this` is unsafe if the object can be destroyed before the callback runs. Cleanup and subscription lifetime must be considered together.

## Backlight

`DisplayInterface::setBrightness(uint8_t)` writes the PWM duty cycle directly. A value of 0 turns the backlight off; 255 is maximum duty. Backlight changes do not stop LVGL or put the device to sleep.

## Current caveats

- Some legacy Wi-Fi/provisioning paths create LVGL timers or acquire the display mutex from non-UI callbacks. Do not copy those paths when adding new behavior; use the UI callback queue.
- `CardController::setDisplayInterface()` is called before `CardNavigationStack` is created, so the stack's mutex pointer is not currently assigned during normal initialization.
