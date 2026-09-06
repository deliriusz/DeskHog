# DeskHog technical details

DeskHog's technical documentation is organized by subsystem under [`docs/`](docs/), with its index and agent guidance in [`AGENTS.md`](AGENTS.md).

Start with the [agent reference](AGENTS.md), then follow the guide for the code you are changing. The most common entry points are:

- [Runtime and FreeRTOS tasks](docs/runtime-and-tasks.md)
- [Display and LVGL](docs/display-and-lvgl.md)
- [Cards](docs/cards.md)
- [Input and navigation](docs/input-and-navigation.md)
- [Wi-Fi and provisioning](docs/wifi-and-provisioning.md)
- [PostHog insights](docs/posthog-insights.md)
- [Games](docs/games.md)

The essential constraint remains: event callbacks and background tasks do not automatically execute on the LVGL task. Marshal UI changes through the UI callback queue.
