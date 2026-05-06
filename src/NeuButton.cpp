#include "NeuButton.h"

/**
 * @brief Constructor
 */
NeuButton::NeuButton(const uint8_t *pins, uint8_t count)
{
    _pins = pins;
    _count = (count > 32) ? 32 : count;

    _inverse = new bool[_count]();
    _pressTime = new unsigned long[_count]();
    _lastRepeatTime = new unsigned long[_count]();
    _repeats = new SpecialEntry[_count]();
    _longPresses = new SpecialEntry[_count]();
    _combines = new CombineEntry[10](); // max 10 combinations

    for (uint8_t i = 0; i < _count; i++)
    {
        pinMode(_pins[i], INPUT_PULLUP);
    }
}

/**
 * @brief Destructor
 */
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
// PUBLIC API
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
    if (_combineCount < 10)
        _combines[_combineCount++] = {cb, mask, exclusive, false};
}

/**
 * @brief Initial state sync (no callbacks)
 */
void NeuButton::refresh()
{
    _state = 0;

    for (uint8_t i = 0; i < _count; i++)
    {
        if ((digitalRead(_pins[i]) == LOW) ^ _inverse[i])
            _state |= (1UL << i);
    }

    _lastState = _state;
    _lastReading = _state;

    // Sync latch with physical state
    _latchState = _state;
}

/**
 * @brief Main processing loop
 */
void NeuButton::loop()
{
    unsigned long now = millis();
    uint32_t reading = 0;

    // --- Scan ---
    for (uint8_t i = 0; i < _count; i++)
    {
        if ((digitalRead(_pins[i]) == LOW) ^ _inverse[i])
            reading |= (1UL << i);
    }

    // --- Debounce ---
    if (reading != _lastReading)
    {
        _lastReading = reading;
        _lastDebounceTime = now;
    }

    if ((now - _lastDebounceTime) < _debounceDelay)
        return;

    // --- Stable state update ---
    if (_state != _lastReading)
    {
        _state = _lastReading;
    }
    else
    {
        goto HOLD_PROCESS;
    }

    uint32_t changed = _state ^ _lastState;

    // =====================================================
    // TRANSITIONS (PRESS / RELEASE / LATCH)
    // =====================================================
    if (changed)
    {
        for (uint8_t i = 0; i < _count; i++)
        {
            uint32_t mask = (1UL << i);

            if (changed & mask)
            {
                if (_state & mask)
                {
                    // PRESS
                    _pressTime[i] = now;
                    _lastRepeatTime[i] = now;
                    _longPresses[i].triggered = false;

                    if (_pressedCb)
                        _pressedCb(i);

                    // LATCH
                    if (_exclusiveLatch)
                        _latchState = mask;
                    else
                        _latchState ^= mask;

                    if (_latchCb)
                        _latchCb(i, (_latchState & mask) != 0);
                }
                else
                {
                    // RELEASE
                    if (_releaseCb)
                        _releaseCb(i);
                }
            }
        }

        // =====================================================
        // COMBINE (EDGE TRIGGERED)
        // =====================================================
        for (uint8_t j = 0; j < _combineCount; j++)
        {
            bool match = (_state & _combines[j].mask) == _combines[j].mask;

            if (_combines[j].exclusive && _state != _combines[j].mask)
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
            {
                _combines[j].active = false;
            }
        }
    }

HOLD_PROCESS:

    // =====================================================
    // HOLD (LONG PRESS + REPEAT)
    // =====================================================
    for (uint8_t i = 0; i < _count; i++)
    {
        if (_state & (1UL << i))
        {
            // LONG PRESS
            if (_longPresses[i].cb && !_longPresses[i].triggered)
            {
                if (now - _pressTime[i] >= _longPresses[i].time)
                {
                    _longPresses[i].cb(i);
                    _longPresses[i].triggered = true;
                }
            }

            // REPEAT
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
