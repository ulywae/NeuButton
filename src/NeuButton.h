#ifndef NEUBUTTON_H
#define NEUBUTTON_H

#include <Arduino.h>

/**
 * @brief Callback for single button events.
 */
typedef void (*ButtonCallback)(uint8_t index);

/**
 * @brief Callback for combination events.
 */
typedef void (*CombineCallback)(uint32_t mask);

/**
 * @brief Callback for latch state changes.
 */
typedef void (*LatchCallback)(uint8_t index, bool latched);

/**
 * @class NeuButton
 * @brief Deterministic multi-button input engine with debounce, latch, repeat, long-press, and combination support.
 *
 * Designed for embedded systems (ESP32/Arduino-class MCUs) with:
 * - Fixed memory usage (no dynamic growth)
 * - Bitmask-based processing (fast and scalable)
 * - Deterministic loop execution
 *
 * Features:
 * - Debounced input scanning
 * - Press / Release events
 * - Latch (toggle or exclusive selector mode)
 * - Long press detection
 * - Repeat events
 * - Multi-button combination detection (edge-triggered)
 */
class NeuButton
{
public:
    /**
     * @brief Construct a new NeuButton instance.
     * @param pins Array of GPIO pins
     * @param count Number of buttons (max 32)
     */
    NeuButton(const uint8_t *pins, uint8_t count);

    /**
     * @brief Destroy the NeuButton instance.
     */
    ~NeuButton();

    // ===== Event Registration =====
    void onPressed(ButtonCallback cb);
    void onRelease(ButtonCallback cb);
    void onLatch(LatchCallback cb);

    void addCombine(CombineCallback cb, uint32_t mask, bool exclusive = false);
    void addRepeat(uint8_t index, uint32_t interval, ButtonCallback cb);
    void addLongPress(uint8_t index, uint32_t duration, ButtonCallback cb);

    // ===== State Access =====
    bool isPressed(uint8_t index);
    bool isLatched(uint8_t index);
    uint32_t getRawState();

    // ===== Configuration =====
    void setInverse(uint8_t index);
    void setExclusiveLatch(bool enable);

    /**
     * @brief Synchronize initial hardware state (boot-time).
     *
     * This reads all pins and initializes internal state and latch state
     * without triggering any callbacks.
     *
     * Useful for:
     * - Selector switches
     * - DIP switches
     * - Power-on state detection
     */
    void refresh();

    /**
     * @brief Main update loop (must be called continuously).
     */
    void loop();

private:
    struct CombineEntry
    {
        CombineCallback cb;
        uint32_t mask;
        bool exclusive;
        bool active; ///< Edge trigger flag
    };

    struct SpecialEntry
    {
        ButtonCallback cb;
        uint32_t time;
        bool triggered;
    };

    const uint8_t *_pins;
    uint8_t _count;

    bool *_inverse;

    uint32_t _state = 0;
    uint32_t _lastState = 0;

    uint32_t _reading = 0;
    uint32_t _lastReading = 0;

    uint32_t _latchState = 0;

    bool _exclusiveLatch = false;

    ButtonCallback _pressedCb = nullptr;
    ButtonCallback _releaseCb = nullptr;
    LatchCallback _latchCb = nullptr;

    SpecialEntry *_repeats;
    SpecialEntry *_longPresses;

    unsigned long *_pressTime;
    unsigned long *_lastRepeatTime;

    unsigned long _lastDebounceTime = 0;
    const unsigned long _debounceDelay = 25;

    CombineEntry *_combines;
    uint8_t _combineCount = 0;
};

#endif
