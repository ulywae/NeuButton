# NeuButton

A lightweight, deterministic, and high-performance Arduino library for managing multiple buttons using **bitmasking**.

NeuButton can handle up to **32 buttons in a single instance** with minimal RAM usage, making it ideal for memory-constrained and real-time applications such as:

- Gamepads
- Macro Pads
- Control Panels
- Embedded UI Systems

Unlike traditional button libraries, NeuButton is designed as a **deterministic input engine**, not just a collection of button objects.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: Arduino](https://img.shields.io/badge/Platform-Arduino-00878F?logo=arduino&logoColor=white)](https://arduino.cc)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue?logo=espressif&logoColor=white)](https://espressif.com)
[![Platform: ESP8266](https://img.shields.io/badge/Platform-ESP8266-lightgrey?logo=espressif&logoColor=white)](https://espressif.com)
[![Architecture: 32-bit](https://img.shields.io/badge/Architecture-32--bit-orange.svg)]()
[![Max Buttons: 32](https://img.shields.io/badge/Max%20Buttons-32-informational.svg)]()
[![Design: Deterministic](https://img.shields.io/badge/Design-Deterministic-critical.svg)]()

---

## Key Features

- **Ultra-Low RAM Footprint**
  Uses 60% less RAM than traditional libraries by avoiding object-oriented overhead and using bit-packing.

- **Auto-Adaptive Bitmask Engine**  
  Intelligently chooses the smallest data type (uint8_t, uint16_t, or uint32_t) based on your button count to ensure peak CPU performance.

- **Zero-Cost Abstractions**  
  Powered by C++ Templates. All logic is resolved at compile-time, resulting in faster execution and smaller Flash size.

- **Deterministic Design**  
  Fully predictable behavior with no hidden delays or runtime surprises.

- **Automatic Latching**  
  Built-in toggle logic. Turn momentary buttons into virtual switches instantly.

- **Exclusive Selector Mode**  
  Optional radio-style behavior (only one button active at a time).

- **Edge-Triggered Combinations**  
  Detect multi-button shortcuts without repeated triggering (anti-spam).

- **Independent Callbacks**  
  Separate handlers for Press, Release, Latch, Long Press, and Repeat.

- **Smart Timers (Long Press & Repeat)**  
  Independent handlers for long presses and auto-repeat, optimized with 16-bit precision to save memory.

- **Non-Blocking Debounce**  
  Stable input handling without blocking delays.

- **Cross-Platform**  
  Works on AVR, ESP8266, ESP32, STM32, and other Arduino-compatible boards.

---

## Installation

### Arduino Library Manager (Recommended)

1. In the Arduino IDE, go to **Sketch > Include Library > Manage Libraries...**
2. Search for **NeuButton**
3. Click **Install**

### PlatformIO

```ini
lib_deps = ulywae/NeuButton
```

### Manual Installation

1. Download the repository as `.zip`
2. Open Arduino IDE
3. Go to **Sketch > Include Library > Add .ZIP Library**
4. Select the downloaded file

---

## Usage

### Basic Example

```cpp
#include <NeuButton.h>

const uint8_t pins[] = {2, 3, 4};
constexpr uint8_t numPins = sizeof(pins) / sizeof(pins[0]);

// Template <COUNT, MAX_COMBINES>
// COUNT: 1 to 32 buttons
// MAX_COMBINES: Max number of multi-button shortcuts (default: 4)
NeuButton<numPins> btn(pins);

void setup() {
    Serial.begin(115200);

    btn.onPressed([](uint8_t index) {
        Serial.print("Pressed: ");
        Serial.println(index);
    });

    btn.onLatch([](uint8_t index, bool state) {
        Serial.print("Latch ");
        Serial.print(index);
        Serial.println(state ? " ON" : " OFF");
    });

    // Important: sync initial hardware state
    btn.refresh();
}

void loop() {
    btn.loop();
}
```

> [!IMPORTANT]
>
> `btn.loop()` must be called continuously.
> NeuButton relies on a deterministic update cycle for accurate timing and event detection.
>
> Note on Memory: NeuButton will automatically use 8-bit variables if buttons ≤ 8, 16-bit if buttons ≤ 16, and 32-bit if buttons > 16. This ensures the most optimal use of processor registers.

---

## Advanced Features

### Long Press & Repeat

```cpp
// Trigger after holding button 0 for 2 seconds
btn.addLongPress(0, 2000, [](uint8_t i) {
    Serial.println("Long press!");
});

// Repeat every 100 ms while holding button 1
btn.addRepeat(1, 100, [](uint8_t i) {
    Serial.println("Repeating...");
});
```

---

### Button Combinations (Shortcuts)

```cpp
// Button 0 + Button 2
btn.addCombine([](uint32_t mask) {
    Serial.println("Shortcut!");
}, (1 << 0) | (1 << 2), true);
```

Combinations are **edge-triggered**, meaning they fire only once when the condition becomes active.

---

## Startup Behavior (Boot-Time Sync)

NeuButton provides `refresh()` to synchronize software state with physical input at startup.

Useful for:

- Toggle switches
- DIP switches
- Hardware selectors

```cpp
btn.refresh();
```

Without calling `refresh()`, all buttons start in a neutral state until interaction occurs.

---

## Bitmask Cheat Sheet

| Button Index | Formula  | Value | Binary       | Hex    |
| ------------ | -------- | ----- | ------------ | ------ |
| 0            | `1 << 0` | 1     | `0b00000001` | `0x01` |
| 1            | `1 << 1` | 2     | `0b00000010` | `0x02` |
| 2            | `1 << 2` | 4     | `0b00000100` | `0x04` |
| 3            | `1 << 3` | 8     | `0b00001000` | `0x08` |
| 4            | `1 << 4` | 16    | `0b00010000` | `0x10` |
| 5            | `1 << 5` | 32    | `0b00100000` | `0x20` |
| 6            | `1 << 6` | 64    | `0b01000000` | `0x40` |
| 7            | `1 << 7` | 128   | `0b10000000` | `0x80` |

## Common Multi-Button Examples (Combo)

To get the combination value, simply add the decimal values ​​or use the bitwise OR (|) operator.

| Combination   | Calculation | Mask Value | Binary (8-bit) |
| ------------- | ----------- | ---------- | -------------- |
| Btn 0 + 1     | 1 + 2       | 3          | `0b00000011`   |
| Btn 0 + 2     | 1 + 4       | 5          | `0b00000101`   |
| Btn 1 + 2     | 2 + 4       | 6          | `0b00000110`   |
| Btn 0 + 1 + 2 | 1+2+4       | 7          | `0b00000111`   |
| Btn 0 + 7     | 1+128       | 129        | `0b10000001`   |

### Examples

- Button 0 + 1 → `1 + 2 = 3`
- Button 0 + 2 → `1 + 4 = 5`
- Button 1 + 2 → `2 + 4 = 6`

### How to use in Code

You can use Decimal, Binary, or Bit-shift format as you wish:

```cpp
// All give the SAME result (Button 0 + 2)
btn.addCombine(myCallback, 5);             // Desimal (Simple)
btn.addCombine(myCallback, 0b101);         // Biner (Visual)
btn.addCombine(myCallback, (1<<0)|(1<<2)); // Bit-shift (Pro)
```

> [!TIP]
>
> Use binary literals for readability
> `0b101` → Button 0 + Button 2

---

## API Reference

| Function                          | Description                            |
| --------------------------------- | -------------------------------------- |
| `NeuButton<COUNT, MAX_COMBINES>`  | Initialize with a fixed number of keys |
| `onPressed(cb)`                   | Called when a button is pressed        |
| `onRelease(cb)`                   | Called when a button is released       |
| `onLatch(cb)`                     | Called when latch state changes        |
| `addLongPress(idx, ms, cb)`       | Trigger once after hold                |
| `addRepeat(idx, ms, cb)`          | Trigger repeatedly while held          |
| `addCombine(cb, mask, exclusive)` | Detect button combinations             |
| `setExclusiveLatch(bool)`         | Enable radio-style latch               |
| `isPressed(idx)`                  | Returns physical state                 |
| `isLatched(idx)`                  | Returns latch state                    |
| `refresh()`                       | Sync hardware state                    |

> [!IMPORTANT]
>
> `addLongPress(idx, ms, cb)` & `addRepeat(idx, ms, cb)` → ms uses uint16_t (max 65 seconds) to save RAM.

---

## Why NeuButton?

Most libraries create one object per button, which wastes RAM. NeuButton takes a different approach:

- **Zero-Cost Templates**: Uses C++ Templates to determine array sizes at compile-time. No RAM wasted on dynamic pointers.
- **Auto-Adaptive Masking**: Automatically chooses the smallest data type (`uint8_t`, `uint16_t`, or `uint32_t`) based on the number of buttons you define.
- **Ultra Low RAM**: Uses only ~314 bytes of RAM for 6 buttons (60% less than other popular libraries).
- **No Heap Allocation**: Doesn't use `new` or `malloc`, making it 100% safe from memory fragmentation.

---

## Notes

This is primarily a personal toolkit built for real-world embedded systems.

If it fits your use case — great.
If not — that’s fine too.

It does exactly what it was designed to do.

---

## Author

**Ulywae (@neufa)**
Part of the **NEU Ecosystem**

> _Handcrafted with pure logic._

---
