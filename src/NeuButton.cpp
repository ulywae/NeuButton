/**
 * @file NeuButton.cpp
 * @brief Implementation of NeuButton multi‑button input engine.
 */

#include "NeuButton.h"

// =====================================================
// Construction / Destruction
// =====================================================

NeuButton::NeuButton(const uint8_t *pins, uint8_t count, unsigned long debounceDelay)
    : _pins(pins),
      _count((count > 32) ? 32 : count),
      _debounceDelay(debounceDelay)
{
    // Allocate per‑button arrays
    _inverse = new bool[_count](); // zero‑initialised
    _pressTime = new unsigned long[_count]();
    _lastRepeatTime = new unsigned long[_count]();
    _repeats = new SpecialEntry[_count]();
    _longPresses = new SpecialEntry[_count]();
    _combines = new CombineEntry[MAX_COMBINES]();

    // Initialise all pins with internal pull‑up
    for (uint8_t i = 0; i < _count; i++)
        pinMode(_pins[i], INPUT_PULLUP);
}

NeuButton::~NeuButton()
{
    delete[] _inverse;
    delete[] _pressTime;
    delete[] _lastRepeatTime;
    delete[] _repeats;
    delete[] _longPresses;
    delete[] _combines;
}

// =====================================================
// Public API
// =====================================================

bool NeuButton::isPressed(uint8_t index)
{
    return (index < _count) && (_state & (1UL << index));
}

bool NeuButton::isLatched(uint8_t index)
{
    return (index < _count) && (_latchState & (1UL << index));
}

uint32_t NeuButton::getRawState()
{
    return _state;
}

void NeuButton::onPressed(ButtonCallback cb) { _pressedCb = cb; }
void NeuButton::onRelease(ButtonCallback cb) { _releaseCb = cb; }
void NeuButton::onLatch(LatchCallback cb) { _latchCb = cb; }

void NeuButton::setInverse(uint8_t index)
{
    if (index < _count)
        _inverse[index] = true;
}

void NeuButton::setExclusiveLatch(bool enable)
{
    _exclusiveLatch = enable;
}

void NeuButton::addRepeat(uint8_t index, uint32_t interval, ButtonCallback cb)
{
    if (index < _count)
        _repeats[index] = {cb, interval, false};
}

void NeuButton::addLongPress(uint8_t index, uint32_t duration, ButtonCallback cb)
{
    if (index < _count)
        _longPresses[index] = {cb, duration, false};
}

void NeuButton::addCombine(CombineCallback cb, uint32_t mask, bool exclusive)
{
    if (_combineCount < MAX_COMBINES)
        _combines[_combineCount++] = {cb, mask, exclusive, false};
}

void NeuButton::refresh()
{
    _state = 0;

    for (uint8_t i = 0; i < _count; i++)
    {
        bool active = (digitalRead(_pins[i]) == LOW) ^ _inverse[i];
        if (active)
            _state |= (1UL << i);
    }

    _lastState = _state;
    _lastReading = _state;
    _latchState = _state; // synchronise latch with physical state
}

// =====================================================
void NeuButton::loop()
{
    unsigned long now = millis();

    // ---- 1. Scan all pins ----
    uint32_t reading = 0;
    for (uint8_t i = 0; i < _count; i++)
    {
        bool active = (digitalRead(_pins[i]) == LOW) ^ _inverse[i];
        if (active)
            reading |= (1UL << i);
    }

    // ---- 2. Debounce ----
    if (reading != _lastReading)
    {
        _lastReading = reading;
        _lastDebounceTime = now;
    }

    if ((now - _lastDebounceTime) < _debounceDelay)
        return; // still bouncing, do nothing

    // ---- 3. Stable state reached ----
    if (_state != _lastReading)
        _state = _lastReading;
    else
        // No change in press state → only handle repeat/long‑press on held buttons
        goto handle_holds;

    // ---- 4. State changed: process press / release transitions ----
    {
        uint32_t changed = _state ^ _lastState;

        if (changed)
        {
            for (uint8_t i = 0; i < _count; i++)
            {
                uint32_t mask = (1UL << i);
                if (changed & mask)
                {
                    if (_state & mask)
                    {
                        // --- Press event ---
                        _pressTime[i] = now;
                        _lastRepeatTime[i] = now;
                        _longPresses[i].triggered = false;

                        if (_pressedCb)
                            _pressedCb(i);

                        // --- Latch handling ---
                        if (_exclusiveLatch)
                            _latchState = mask; // only this button latched
                        else
                            _latchState ^= mask; // toggle

                        if (_latchCb)
                            _latchCb(i, (_latchState & mask) != 0);
                    }
                    else
                    {
                        // --- Release event ---
                        if (_releaseCb)
                            _releaseCb(i);
                    }
                }
            }
        }

        // ---- 5. Combination detection (edge‑triggered) ----
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
                        _combines[j].cb(_state);
                    _combines[j].active = true;
                }
            }
            else
                _combines[j].active = false;
        }
    }

handle_holds:
    // ---- 6. Handle long press and repeat for held buttons ----
    for (uint8_t i = 0; i < _count; i++)
    {
        if (_state & (1UL << i))
        {
            // Long press
            if (_longPresses[i].cb && !_longPresses[i].triggered)
            {
                if (now - _pressTime[i] >= _longPresses[i].time)
                {
                    _longPresses[i].cb(i);
                    _longPresses[i].triggered = true;
                }
            }

            // Repeat
            if (_repeats[i].cb)
            {
                if (now - _lastRepeatTime[i] >= _repeats[i].time)
                {
                    _repeats[i].cb(i);
                    _lastRepeatTime[i] = now;
                }
            }
        }
    }

    _lastState = _state;
}
