# Tamagotchi Phase 7 execution record

## Result

Implemented the platform-neutral deterministic model in
`src/tamagotchi/TamagotchiModel.h` and `src/tamagotchi/TamagotchiModel.cpp`.
The model owns its state and dirty mask by value, has no platform, UI, storage,
clock, task, logging, randomness, exception, or allocation dependency, and
leaves all Phase 8-13 integration work unimplemented.

## Host validation

The temporary host harness was compiled and run outside the repository with:

```sh
g++ -std=gnu++17 -Wall -Wextra -Werror -pedantic -Isrc \
  /tmp/tamagotchi_model_harness.cpp src/tamagotchi/TamagotchiModel.cpp
```

It exercised fresh state, hatch/re-hatch, egg refusal/advance, all care actions
and refusals, need clamps, the first mess, threshold ordering, Doctor relapse,
evolution, dirty-mask clearing, epoch validation, stable-state fast-forward,
and bulk-versus-partitioned advancement across cadence boundaries. A second
run with UBSan (`-fsanitize=undefined -fno-sanitize-recover=all`) completed
without diagnostics. The dependency output contains no Arduino, LVGL,
Preferences, or ArduinoJson headers.

`TamagotchiTypes.h`'s existing multi-statement `constexpr operator|=` requires
C++14 or newer, so a C++11 host compile fails before the model is compiled.
The firmware and host harness were validated in C++17; Phase 2's shared header
was not changed.

At the exact `UINT64_MAX` elapsed-time edge, fixed 60-second quanta preserve a
15-second remainder. From age zero, the processed age is `UINT64_MAX - 15`;
it cannot reach the final 15 seconds without another complete quantum. With an
existing remainder of 59, the carry creates one additional minute, saturating
age at `UINT64_MAX` and leaving remainder 14. Both cases terminate promptly,
reach the stable Adult state, and preserve deterministic state semantics.

## Production build

```sh
~/.platformio/penv/bin/pio run -s -e adafruit_feather_esp32s3_reversetft
```

Completed successfully. The build regenerated its portal and sprite sources,
but they were deterministic and left no working-tree changes. The resulting
firmware is 1,956,560 bytes (SHA-256
`3ec80c5344c8021808f5ae5e3eaef3b0bd0c2abb6adf75c560d3a0549c9208fb`),
unchanged from Phase 6 because the model has no caller yet and is discarded by
the release link. The OTA-slot headroom remains 75,056 bytes.

The build continues to report pre-existing `src_filter` deprecation, Flappy
LVGL enum-conversion, and OTA timeout narrowing warnings. No warning came from
the Phase 7 model.

## Scope review

The only source additions are the two model files. No LVGL, Arduino, NVS,
clock, portal, asset, or task code was changed, and no native PlatformIO test
environment was introduced.
