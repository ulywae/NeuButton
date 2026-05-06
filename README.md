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

- **Bitmask Engine**  
  Process up to 32 buttons using a single `uint32_t` state. Fast and memory-efficient.

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
````

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
const uint8_t numPins = sizeof(pins) / sizeof(pins[0]);

NeuButton btn(pins, numPins);

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

* Toggle switches
* DIP switches
* Hardware selectors

```cpp
btn.refresh();
```

Without calling `refresh()`, all buttons start in a neutral state until interaction occurs.

---

## Bitmask Cheat Sheet

| Button Index | Formula  | Value | Binary  |
| ------------ | -------- | ----- | ------- |
| 0            | `1 << 0` | 1     | `00001` |
| 1            | `1 << 1` | 2     | `00010` |
| 2            | `1 << 2` | 4     | `00100` |
| 3            | `1 << 3` | 8     | `01000` |
| 4            | `1 << 4` | 16    | `10000` |

### Examples

* Button 0 + 1 → `1 + 2 = 3`
* Button 0 + 2 → `1 + 4 = 5`
* Button 1 + 2 → `2 + 4 = 6`

> [!TIP]
>
> Use binary literals for readability
> `0b101` → Button 0 + Button 2

---

## API Reference

| Function                          | Description                      |
| --------------------------------- | -------------------------------- |
| `onPressed(cb)`                   | Called when a button is pressed  |
| `onRelease(cb)`                   | Called when a button is released |
| `onLatch(cb)`                     | Called when latch state changes  |
| `addLongPress(idx, ms, cb)`       | Trigger once after hold          |
| `addRepeat(idx, ms, cb)`          | Trigger repeatedly while held    |
| `addCombine(cb, mask, exclusive)` | Detect button combinations       |
| `setExclusiveLatch(bool)`         | Enable radio-style latch         |
| `isPressed(idx)`                  | Returns physical state           |
| `isLatched(idx)`                  | Returns latch state              |
| `refresh()`                       | Sync hardware state              |

---

## Why NeuButton?

Most button libraries create **one object per button**, increasing RAM usage and complexity.

NeuButton uses a different approach:

* Single engine
* Bitmask processing
* Deterministic execution

Result:

* Lower memory usage
* Faster execution
* Built-in advanced logic (Latch, Combine, Selector)

It scales cleanly as your project grows.

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

> *Handcrafted with pure logic.*

---

Developed with ❤️ for the Arduino Community.
