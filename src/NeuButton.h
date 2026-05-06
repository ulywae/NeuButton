/**
 * @file NeuButton.h
 * @brief Deterministic multi-button input engine for embedded systems.
 * @author Ulywae
 *
 * Supports debounce, latch, repeat, long-press, and multi-button combinations.
 * Designed for Arduino/ESP32 with bitmask-based processing.
 */

#ifndef NEUBUTTON_H
#define NEUBUTTON_H

#include <Arduino.h>

/**
 * @brief Callback for single button events (press, release, repeat, long press).
 * @param index Button index (0..count-1)
 */
typedef void (*ButtonCallback)(uint8_t index);

/**
 * @brief Callback for combination events.
 * @param mask Bitmask of currently pressed buttons that triggered the combination.
 */
typedef void (*CombineCallback)(uint32_t mask);

/**
 * @brief Callback for latch state changes.
 * @param index Button index
 * @param latched true if latched on, false if latched off
 */
typedef void (*LatchCallback)(uint8_t index, bool latched);

/**
 * @class NeuButton
 * @brief Multi-button input engine with deterministic behavior.
 *
 * Features:
 * - Debounced scanning
 * - Press/release events
 * - Latch (toggle or exclusive selector mode)
 * - Long press detection
 * - Repeat events (auto-fire)
 * - Multi-button combination detection (edge-triggered)
 *
 * Memory is allocated once in the constructor and freed in the destructor.
 * Maximum number of buttons: 32 (fits in uint32_t bitmask).
 */
class NeuButton
{
public:
    /**
     * @brief Construct a new NeuButton object.
     *
     * @param pins Array of GPIO pin numbers. Must remain valid for the lifetime of this object.
     * @param count Number of buttons (max 32). If >32, only first 32 are used.
     * @param debounceDelay Debounce time in milliseconds (default: 25ms).
     *
     * @note The pins are configured as INPUT_PULLUP internally.
     * @note If your hardware uses pull-down or external pull-ups, use setInverse().
     */
    NeuButton(const uint8_t *pins, uint8_t count, unsigned long debounceDelay = 25);

    /// Destructor – releases all dynamically allocated memory.
    ~NeuButton();

    // ===== Event Registration =====

    /**
     * @brief Set callback for button press events.
     * @param cb Function called when any button becomes pressed (edge-triggered).
     */
    void onPressed(ButtonCallback cb);

    /**
     * @brief Set callback for button release events.
     * @param cb Function called when any button becomes released.
     */
    void onRelease(ButtonCallback cb);

    /**
     * @brief Set callback for latch state changes.
     * @param cb Function called whenever a button's latch state changes.
     */
    void onLatch(LatchCallback cb);

    /**
     * @brief Register a multi-button combination.
     * @param cb Function called when the exact button combination becomes active (edge-triggered).
     * @param mask Bitmask of buttons (bits 0..count-1) that must be pressed.
     * @param exclusive If true, the combination only triggers when exactly those buttons are pressed (no others).
     *
     * @note Maximum 10 combinations. Additional calls will be ignored.
     */
    void addCombine(CombineCallback cb, uint32_t mask, bool exclusive = false);

    /**
     * @brief Register a repeat (auto-fire) event for a button.
     * @param index Button index.
     * @param interval Repeat interval in milliseconds.
     * @param cb Function called repeatedly while button is held, after the initial press.
     */
    void addRepeat(uint8_t index, uint32_t interval, ButtonCallback cb);

    /**
     * @brief Register a long press event for a button.
     * @param index Button index.
     * @param duration Milliseconds after which the callback is triggered (once per press).
     * @param cb Function called when the button is held longer than duration.
     */
    void addLongPress(uint8_t index, uint32_t duration, ButtonCallback cb);

    // ===== State Access =====

    /**
     * @brief Get the current physical press state of a button (debounced).
     * @param index Button index.
     * @return true if button is currently pressed down.
     */
    bool isPressed(uint8_t index);

    /**
     * @brief Get the current latched state of a button.
     * @param index Button index.
     * @return true if button is latched (toggled on).
     */
    bool isLatched(uint8_t index);

    /**
     * @brief Get raw bitmask of all currently pressed buttons.
     * @return uint32_t bitmask where bit i corresponds to button i.
     */
    uint32_t getRawState();

    // ===== Configuration =====

    /**
     * @brief Invert (reverse) the logic for a specific button.
     * @param index Button index.
     *
     * Use this if your button is wired as active HIGH (e.g., external pull‑down).
     * By default all buttons are active LOW (internal pull‑up).
     *
     * @note This only affects logical interpretation, not pin configuration.
     */
    void setInverse(uint8_t index);

    /**
     * @brief Enable or disable exclusive (radio‑button) latch mode.
     * @param enable true = exclusive mode, false = normal toggle mode.
     *
     * - Normal (false): each button toggles its own latch independently.
     * - Exclusive (true): pressing a button latches it on and turns off all others.
     */
    void setExclusiveLatch(bool enable);

    /**
     * @brief Synchronize initial hardware state (boot‑time).
     *
     * Reads all pins and initialises internal state and latch state
     * without triggering any callbacks. Call this once in setup().
     */
    void refresh();

    /**
     * @brief Main update loop – must be called frequently (e.g., in loop()).
     *
     * Processes debouncing, state changes, and triggers callbacks.
     *
     * @note Should be called as often as possible (e.g., every loop iteration).
     */
    void loop();

private:
    /// Maximum number of simultaneous button combinations that can be registered.
    static constexpr uint8_t MAX_COMBINES = 10;

    /// Structure for combination storage.
    struct CombineEntry
    {
        CombineCallback cb; ///< User callback
        uint32_t mask;      ///< Required button bitmask
        bool exclusive;     ///< If true, only exact match triggers
        bool active;        ///< Edge trigger flag (true if already triggered)
    };

    /// Structure for repeat and long press shared fields.
    struct SpecialEntry
    {
        ButtonCallback cb; ///< User callback
        uint32_t time;     ///< Interval (repeat) or duration (long press)
        bool triggered;    ///< True if long press already fired for this press
    };

    const uint8_t *_pins; ///< GPIO pin numbers (user‑supplied array)
    uint8_t _count;       ///< Number of buttons (capped at 32)

    bool *_inverse; ///< Per‑button inversion flag

    uint32_t _state = 0;     ///< Current debounced press state
    uint32_t _lastState = 0; ///< Previous debounced press state

    uint32_t _lastReading = 0; ///< Last raw reading for debouncing
    uint32_t _latchState = 0;  ///< Current latched state

    bool _exclusiveLatch = false; ///< Exclusive latch mode flag

    ButtonCallback _pressedCb = nullptr; ///< Global press callback
    ButtonCallback _releaseCb = nullptr; ///< Global release callback
    LatchCallback _latchCb = nullptr;    ///< Global latch callback

    SpecialEntry *_repeats;     ///< Per‑button repeat configuration (array size _count)
    SpecialEntry *_longPresses; ///< Per‑button long press configuration

    unsigned long *_pressTime;      ///< Timestamp when each button was pressed
    unsigned long *_lastRepeatTime; ///< Last time repeat callback was called

    unsigned long _lastDebounceTime = 0; ///< Last time raw state changed
    unsigned long _debounceDelay;        ///< Debounce delay in ms

    CombineEntry *_combines;   ///< Array of registered combinations (size MAX_COMBINES)
    uint8_t _combineCount = 0; ///< Current number of registered combinations
};

#endif // NEUBUTTON_H
