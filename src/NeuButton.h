/**
 * @file NeuButton.h
 * @brief Ultra-low RAM, deterministic multi-button input engine.
 * @author Ulywae (@neufa)
 *
 * Features:
 * - Zero-cost abstractions using C++ Templates.
 * - Auto-adaptive bitmask engine (8, 16, or 32-bit) for maximum efficiency.
 * - Optimized with bit-packing and 16-bit smart timers.
 * - Supports Debounce, Latch (Toggle/Exclusive), Long Press, Repeat,
 *   and Edge-triggered Multi-button Combinations.
 *
 * Designed for memory-constrained Arduino/ESP32/STM32 projects.
 */

#ifndef NEUBUTTON_H
#define NEUBUTTON_H

#include <Arduino.h>

template <bool Condition, typename T, typename F>
struct TypeIf
{
    typedef T type;
};
template <typename T, typename F>
struct TypeIf<false, T, F>
{
    typedef F type;
};

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
 * @param index Button index that toggled its latch state.
 * @param latched True if the virtual switch is now ON, false if OFF.
 */
typedef void (*LatchCallback)(uint8_t index, bool latched);

/**
 * @class NeuButton
 * @brief Ultra-low RAM, deterministic multi-button input engine.
 *
 * Features:
 * - Zero-cost abstractions using C++ Templates.
 * - Auto-adaptive bitmask engine (8, 16, or 32-bit) based on button count.
 * - Deterministic, non-blocking debounce scanning.
 * - Events: Press, Release, Latch (Toggle/Exclusive), Long Press, and Auto-Repeat.
 * - Edge-triggered multi-button combination detection (Shortcut engine).
 *
 * Memory:
 * - 100% Static Allocation (Zero Heap usage).
 * - Optimized with bit-packing and 16-bit smart timers.
 */
template <uint8_t COUNT, uint8_t MAX_COMBINES = 4>
class NeuButton
{
private:
    /**
     * @brief Adaptive bitmask type.
     * Automatically scales to uint8_t, uint16_t, or uint32_t based on COUNT.
     */
    typedef typename TypeIf<(COUNT <= 8), uint8_t,
                            typename TypeIf<(COUNT <= 16), uint16_t, uint32_t>::type>::type MaskType;

    /**
     * @brief Storage for timed button events.
     * Optimized using bit-fields and 16-bit timers to save RAM.
     */
    struct SpecialEntry
    {
        ButtonCallback cb = nullptr; ///< Pointer to the user-defined callback function
        uint16_t time = 0;           ///< Duration (long press) or Interval (repeat) in ms
        uint8_t triggered : 1;       ///< 1-bit flag: True if the event has already fired
    };

    /**
     * @brief Storage for multi-button combination shortcuts.
     */
    struct CombineEntry
    {
        CombineCallback cb = nullptr; ///< Pointer to the combination callback function
        MaskType mask = 0;            ///< The specific bitmask that triggers this combo
        uint8_t exclusive : 1;        ///< 1-bit flag: If true, no other buttons must be pressed
        uint8_t active : 1;           ///< 1-bit flag: Prevents repeated triggering while held
    };

    uint8_t _pins[COUNT];      ///< Array of physical GPIO pin numbers
    MaskType _inverseMask = 0; ///< Mask for buttons that are Active-High (Inverse)
    MaskType _state = 0;       ///< Current debounced state of all buttons (bitmask)
    MaskType _lastState = 0;   ///< Previous debounced state for edge detection
    MaskType _lastReading = 0; ///< Raw pin reading used during debouncing
    MaskType _latchState = 0;  ///< Virtual toggle/latch state for each button

    uint16_t _pressTime[COUNT];      ///< Compressed timestamp (16-bit) of the last press event
    uint16_t _lastRepeatTime[COUNT]; ///< Compressed timestamp (16-bit) of the last repeat trigger

    SpecialEntry _repeats[COUNT];         ///< Array of repeat event configurations
    SpecialEntry _longPresses[COUNT];     ///< Array of long press event configurations
    CombineEntry _combines[MAX_COMBINES]; ///< Array of registered button combinations

    uint8_t _combineCount = 0;           ///< Current number of registered combinations
    unsigned long _lastDebounceTime = 0; ///< Global timestamp for the debounce timer
    uint8_t _debounceDelay;              ///< Configurable debounce interval (ms)
    uint8_t _exclusiveLatch : 1 = 0;     ///< 1-bit flag: Enable/disable radio-style latch mode

    ButtonCallback _pressedCb = nullptr; ///< Global callback for any button press
    ButtonCallback _releaseCb = nullptr; ///< Global callback for any button release
    LatchCallback _latchCb = nullptr;    ///< Global callback for any latch state change

public:
    /**
     * @brief Construct a new NeuButton object.
     *
     * @tparam COUNT Number of pins to manage (determined at compile-time).
     * @tparam MAX_COMBINES Maximum slots for multi-button shortcuts (default: 4).
     *
     * @param pins Array of GPIO pin numbers.
     * @param debounceDelay Debounce time in milliseconds (default: 50ms).
     *
     * @note Pins are automatically configured as INPUT_PULLUP.
     * @note Use setInverse(index) if your hardware is Active-High (External Pull-Down).
     * @note Memory is statically allocated based on the <COUNT> template parameter.
     */
    NeuButton(const uint8_t pins[COUNT], uint8_t debounceDelay = 50)
        : _debounceDelay(debounceDelay)
    {
        for (uint8_t i = 0; i < COUNT; i++)
        {
            _pins[i] = pins[i];
            pinMode(_pins[i], INPUT_PULLUP);
            _pressTime[i] = 0;
            _lastRepeatTime[i] = 0;
            _repeats[i].triggered = 0;
            _longPresses[i].triggered = 0;
        }
    }

    // ===== Event Registration =====

    /**
     * @brief Set callback for button press events.
     * @param cb Function called when any button is pressed (edge-triggered).
     */
    void onPressed(ButtonCallback cb) { _pressedCb = cb; }

    /**
     * @brief Set callback for button release events.
     * @param cb Function called when any button is released.
     */
    void onRelease(ButtonCallback cb) { _releaseCb = cb; }

    /**
     * @brief Set callback for latch state changes.
     * @param cb Function called whenever a button's latch state changes.
     */
    void onLatch(LatchCallback cb) { _latchCb = cb; }

    /**
     * @brief Register a long press event for a button.
     * @param index Button index (0..count-1).
     * @param ms Duration milliseconds (max 65535ms) after which the callback is triggered (once per press).
     * @param cb Function called when the button is held longer than duration.
     */
    void addLongPress(uint8_t index, uint16_t ms, ButtonCallback cb)
    {
        if (index < COUNT)
        {
            _longPresses[index].cb = cb;
            _longPresses[index].time = ms;
        }
    }

    /**
     * @brief Register a repeat (auto-fire) event for a button.
     * @param index Button index (0..count-1).
     * @param interval Repeat interval in milliseconds.
     * @param cb Function called repeatedly while button is held, after the initial press.
     */
    void addRepeat(uint8_t index, uint16_t interval, ButtonCallback cb)
    {
        if (index < COUNT)
        {
            _repeats[index].cb = cb;
            _repeats[index].time = interval;
        }
    }

    /**
     * @brief Register a multi-button combination (Shortcut).
     *
     * @param cb Function called when the exact button combination becomes active (edge-triggered).
     * @param mask Bitmask of buttons that must be pressed (e.g., (1<<0) | (1<<2)).
     * @param exclusive If true, triggers only when ONLY these buttons are pressed (no others).
     *
     * @note Maximum slots are defined by the MAX_COMBINES template parameter (default: 4).
     * @note Combinations are edge-triggered; they fire once and won't repeat until released.
     */
    void addCombine(CombineCallback cb, uint32_t mask, bool exclusive = false)
    {
        if (_combineCount < MAX_COMBINES)
        {
            _combines[_combineCount].cb = cb;
            _combines[_combineCount].mask = (MaskType)mask;
            _combines[_combineCount].exclusive = exclusive;
            _combineCount++;
        }
    }

    // ===== State Access =====

    /**
     * @brief Get the current physical press state of a button (debounced).
     * @param index Button index (0..count-1).
     * @return true if button is currently pressed down.
     */
    bool isPressed(uint8_t index) { return (index < COUNT) && (_state & ((MaskType)1 << index)); }

    /**
     * @brief Get the current latched state of a button.
     * @param index Button index (0..count-1).
     * @return true if button is latched (toggled on).
     */
    bool isLatched(uint8_t index) { return (index < COUNT) && (_latchState & ((MaskType)1 << index)); }

    /**
     * @brief Get raw bitmask of all currently pressed buttons.
     * @return uint32_t bitmask where bit i corresponds to button i.
     */
    uint32_t getRawState() { return (uint32_t)_state; }

    // ===== Configuration =====

    /**
     * @brief Invert (reverse) the logic for a specific button.
     * @param index Button index (0..count-1).
     *
     * Use this if your button is wired as active HIGH (e.g., external pull‑down).
     * By default all buttons are active LOW (internal pull‑up).
     *
     * @note This only affects logical interpretation, not pin configuration.
     */
    void setInverse(uint8_t index)
    {
        if (index < COUNT)
            _inverseMask |= ((MaskType)1 << index);
    }

    /**
     * @brief Enable or disable exclusive (radio‑button) latch mode.
     * If true, only one button can be latched at a time.
     * @param enable true = exclusive mode, false = normal toggle mode.
     *
     * - Normal (false): each button toggles its own latch independently.
     * - Exclusive (true): pressing a button latches it on and turns off all others.
     */
    void setExclusiveLatch(bool enable) { _exclusiveLatch = enable; }

    /**
     * @brief Re-synchronize internal state with physical button levels.
     *
     * This function reads all pins and updates _state, _lastState, _lastReading,
     * and _latchState to match the current physical conditions.
     *
     * Use cases:
     * - Called once in setup() to initialize.
     * - Called after waking from sleep or resuming from an inactive mode
     *   where button states might have changed while the system was ignoring them.
     *
     * @warning This will clear any latched states that were set programmatically.
     *          Only call when you intend to discard previous latch information.
     */
    void refresh()
    {
        _state = 0;
        for (uint8_t i = 0; i < COUNT; i++)
        {
            bool inv = (_inverseMask >> i) & 1;
            if ((digitalRead(_pins[i]) == LOW) ^ inv)
                _state |= ((MaskType)1 << i);
        }
        _lastState = _state;
        _lastReading = _state;
        _latchState = _state;
    }

    /**
     * @brief Main update loop – must be called frequently (e.g., in loop()).
     *
     * Processes debouncing, state changes, and triggers callbacks.
     *
     * @note Should be called as often as possible (e.g., every loop iteration).
     */
    void loop()
    {
        unsigned long now = millis();
        uint16_t now16 = (uint16_t)now;
        MaskType reading = 0;

        for (uint8_t i = 0; i < COUNT; i++)
        {
            bool inv = (_inverseMask >> i) & 1;
            if ((digitalRead(_pins[i]) == LOW) ^ inv)
                reading |= ((MaskType)1 << i);
        }

        if (reading != _lastReading)
        {
            _lastReading = reading;
            _lastDebounceTime = now;
        }

        if ((now - _lastDebounceTime) < _debounceDelay)
            return;

        if (_state != _lastReading)
        {
            _state = _lastReading;
        }
        else
        {
            goto handle_holds;
        }

        {
            MaskType changed = _state ^ _lastState;
            if (changed)
            {
                for (uint8_t i = 0; i < COUNT; i++)
                {
                    MaskType mask = ((MaskType)1 << i);
                    if (changed & mask)
                    {
                        if (_state & mask)
                        { // Pressed
                            _pressTime[i] = now16;
                            _lastRepeatTime[i] = now16;
                            _longPresses[i].triggered = 0;
                            if (_pressedCb)
                                _pressedCb(i);
                            if (_exclusiveLatch)
                                _latchState = mask;
                            else
                                _latchState ^= mask;
                            if (_latchCb)
                                _latchCb(i, (_latchState & mask) != 0);
                        }
                        else
                        { // Released
                            if (_releaseCb)
                                _releaseCb(i);
                        }
                    }
                }
            }

            for (uint8_t j = 0; j < _combineCount; j++)
            {
                bool match = (_state & _combines[j].mask) == _combines[j].mask;
                if (_combines[j].exclusive && (_state != _combines[j].mask))
                    match = false;
                if (match)
                {
                    if (!_combines[j].active)
                    {
                        if (_combines[j].cb)
                            _combines[j].cb((uint32_t)_state);
                        _combines[j].active = 1;
                    }
                }
                else
                    _combines[j].active = 0;
            }
        }

    handle_holds:
        for (uint8_t i = 0; i < COUNT; i++)
        {
            if (_state & ((MaskType)1 << i))
            {
                uint16_t elapsed = now16 - _pressTime[i];
                if (_longPresses[i].cb && !_longPresses[i].triggered)
                {
                    if (elapsed >= _longPresses[i].time)
                    {
                        _longPresses[i].cb(i);
                        _longPresses[i].triggered = 1;
                    }
                }
                if (_repeats[i].cb)
                {
                    if ((uint16_t)(now16 - _lastRepeatTime[i]) >= _repeats[i].time)
                    {
                        _repeats[i].cb(i);
                        _lastRepeatTime[i] = now16;
                    }
                }
            }
        }
        _lastState = _state;
    }
};

#endif
