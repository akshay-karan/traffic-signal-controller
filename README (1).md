# 3-Way Traffic Signal Controller (STM32 F446RE)

A 3-way traffic signal controller with adaptive green timing, built for the STM32 Nucleo-64 F446RE using the Arduino IDE. Green light duration responds to real-time traffic presence via IR sensors, and an onboard LED adjusts brightness to ambient light using a photoresistor.

## Features

- **Adaptive green timing** — each signal defaults to a 5s green phase, but extends instantly to 10s if an IR sensor detects a waiting vehicle. Only extends once per phase, then resets back to 5s for the next cycle.
- **Overlapping yellow handoff** — the outgoing signal's yellow and the incoming signal's yellow run together during the transition, rather than a strict red-then-green switch.
- **Ambient light response** — a photoresistor (LDR) drives the onboard LED (PA5) via PWM: dim in bright rooms, bright in the dark.
- **Live serial countdown** — every phase (green/yellow) prints a per-second countdown, IR status, and LDR/brightness readings to the serial monitor.

## Hardware

| Signal | Red | Yellow | Green |
|---|---|---|---|
| Signal 1 | A1 (PA1) | D12 | D11 |
| Signal 2 | D10 | D9 | D8 |
| Signal 3 | D7 | D6 | D5 |

| Sensor | Pin | Notes |
|---|---|---|
| IR 1 / 2 / 3 | D2 / D3 / D4 | Active-LOW (car detected = LOW); `INPUT_PULLUP` used, no external resistor needed |
| LDR | A0 (PA0) | 12-bit ADC (0–4095) |
| Onboard LED | D13 (PA5) | PWM via TIM2/CH1, used as ambient-light indicator |

## Board setup (Arduino IDE)

1. Install **STM32 MCU based boards** via Board Manager (by STMicroelectronics).
2. Select:
   - **Board:** Nucleo-64
   - **Board part number:** Nucleo F446RE
   - **Upload method:** STLink
3. Open the `.ino` file and upload.

## Notes on porting from Arduino Uno

This controller was adapted from an Arduino Uno version, with a few STM32-specific changes:

- **ADC resolution:** F446RE's ADC is 12-bit (0–4095) vs. the Uno's 10-bit (0–1023) — `analogReadResolution(12)` is set explicitly, and LDR thresholds are scaled ×4 accordingly (800→3200, 200→800).
- **PWM resolution:** kept at 8-bit (`analogWriteResolution(8)`, 0–255) to match existing `analogWrite()` calls.
- Pin names use the Arduino-style `D`/`A` aliases (e.g. `D12`, `A1`) mapped to their STM32 port/pin equivalents (e.g. PA1) in comments.

## Serial output example

```
=== Traffic Controller Started (STM32 F446RE) ===
>> Signal 1 GREEN  |  5s
  S1 GREEN 5s  |  IR: clear  |  LDR: 2100  LED brightness: 108
  *** S1 car detected → extended to 10s ***
  S1 GREEN 9s  |  IR: DETECTED  |  LDR: 2094  LED brightness: 107
```

## License

MIT — feel free to fork and adapt.
