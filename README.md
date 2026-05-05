# NeuButton

A lightweight, high-performance Arduino library for managing multiple buttons using **Bitmasking**. NeuButton is designed to handle up to 32 buttons with minimal RAM consumption, making it ideal for memory-constrained projects like Gamepads, Macro Pads, and complex Control Panels.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: Arduino](https://img.shields.io/badge/Platform-Arduino-00878F?logo=arduino&logoColor=white)](https://arduino.cc)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue?logo=espressif&logoColor=white)](https://espressif.com)
[![Platform: ESP8266](https://img.shields.io/badge/Platform-ESP8266-lightgrey?logo=espressif&logoColor=white)](https://espressif.com)
[![Architecture: 32-bit](https://img.shields.io/badge/Architecture-32--bit-orange.svg)]()
[![Max Buttons: 32](https://img.shields.io/badge/Max%20Buttons-32-informational.svg)]()
[![Design: Deterministic](https://img.shields.io/badge/Design-Deterministic-critical.svg)]()

---

## Key Features

- **Bitmask Engine**: Manages up to 32 buttons using a single `uint32_t` variable.
- **Automatic Latching**: Built-in toggle logic. Turn any momentary button into a virtual switch without extra code.
- **Powerful Combinations**: Detect multi-button shortcuts (e.g., Btn A + Btn B) with an optional exclusive mode.
- **Independent Callbacks**: Separate events for Pressed, Released, Latching, Long Press, and Repeat.
- **Hardware Debouncing**: Non-blocking software debouncing for stable signals.
- **Universal Compatibility**: Works on all Arduino-compatible boards (AVR, ESP8266, ESP32, STM32, etc.).

---

## Installation

### Arduino Library Manager (Recommended)

1. In the Arduino IDE, go to **Sketch > Include Library > Manage Libraries...**
2. Search for **NeuButton** in the search bar.
3. Click **Install** on the latest version.

### PlatformIO

Add the following to your `platformio.ini`:

```ini
lib_deps = ulywae/NeuButton
```

### Manual Installation

1. Download the [NeuButton repository](https://github.com) as a `.zip` file.
2. In the Arduino IDE, go to **Sketch > Include Library > Add .ZIP Library**.
3. Select the downloaded file.

---

## Usage

### Basic Setup

```cpp
#include <NeuButton.h>

const uint8_t pins[] = {2, 3, 4}; // Defined button pins
const uint8_t numPins = sizeof(pins) / sizeof(pins[0]);

NeuButton btn(pins, numPins);

void setup() {
    Serial.begin(115200);

    // Register simple press event
    btn.onPressed([](uint8_t index) {
        Serial.print("Button Pressed: ");
        Serial.println(index);
    });

    // Register latching event (Toggle)
    btn.onLatch([](uint8_t index, bool state) {
        Serial.print("Button ");
        Serial.print(index);
        Serial.println(state ? " ON" : " OFF");
    });

    // Sync initial state (Crucial for physical toggle switches)
    btn.refresh();
}

void loop() {
    btn.loop();
}
```

### Advanced Features

#### Long Press & Auto-Repeat

Every button can have its own timing for specific actions.

```cpp
// Trigger after holding button 0 for 2 seconds
btn.addLongPress(0, 2000, [](uint8_t index) {
    Serial.println("Long Press Detected!");
});

// Repeat every 100ms while holding button 1
btn.addRepeat(1, 100, [](uint8_t index) {
    Serial.println("Repeating...");
});
```

#### Button Combinations (Shortcuts)

Detect when multiple buttons are pressed together.

```cpp
// Trigger when Button 0 and Button 2 are pressed simultaneously
// Mask (1 << 0) | (1 << 2) = 5
btn.addCombine([](uint32_t mask) {
    Serial.println("Shortcut Triggered!");
}, 5, true);
```

---

## API Reference

| Function                          | Description                                           |
| :-------------------------------- | :---------------------------------------------------- |
| `onPressed(cb)`                   | Called immediately when a button is pressed.          |
| `onRelease(cb)`                   | Called when a button is released.                     |
| `onLatch(cb)`                     | Called when a button's toggle state changes.          |
| `addLongPress(idx, ms, cb)`       | Single trigger after `ms` duration.                   |
| `addRepeat(idx, ms, cb)`          | Repeated trigger every `ms` while held.               |
| `addCombine(cb, mask, exclusive)` | Triggered on specific button combinations.            |
| `isPressed(idx)`                  | Returns `true` if the button is physically held.      |
| `isLatched(idx)`                  | Returns `true` if the button's toggle state is ON.    |
| `refresh()`                       | Synchronizes software state with physical pin states. |

---

## Why NeuButton?

Most button libraries create an object for every single button, which consumes significant RAM. **NeuButton** uses **Bitwise Logic** to process all buttons at once. It’s faster, smaller, and provides advanced logic (Latch/Combine) out of the box that usually requires manual coding.

---

## Notes

This is primarily a personal toolkit. If it helps your project — great. If not — that’s fine too. It does exactly what it was built to do.

## Author

**Ulywae** (@neufa)  
Part of the **NEU Ecosystem**

> _Handcrafted with pure logic._

---

Developed with ❤️ for the Arduino Community.
