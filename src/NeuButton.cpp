#include "NeuButton.h"

NeuButton::NeuButton(const uint8_t *pins, uint8_t count)
{
    _pins = pins;
    _count = (count > 32) ? 32 : count;

    _inverse = new bool[_count]();
    _pressTime = new unsigned long[_count]();
    _lastRepeatTime = new unsigned long[_count]();
    _repeats = new SpecialEntry[_count]();
    _longPresses = new SpecialEntry[_count]();
    _combines = new CombineEntry[10]; // Limit 10 kombinasi

    for (uint8_t i = 0; i < _count; i++)
    {
        pinMode(_pins[i], INPUT_PULLUP);
    }
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

// Getters
bool NeuButton::isPressed(uint8_t index) { return (index < _count) && (_state & (1UL << index)); }
bool NeuButton::isLatched(uint8_t index) { return (index < _count) && (_latchState & (1UL << index)); }
uint32_t NeuButton::getRawState() { return _state; }

// Config & Callbacks
void NeuButton::onPressed(ButtonCallback cb) { _pressedCb = cb; }
void NeuButton::onRelease(ButtonCallback cb) { _releaseCb = cb; }
void NeuButton::onLatch(LatchCallback cb) { _latchCb = cb; }
void NeuButton::setInverse(uint8_t index)
{
    if (index < _count)
        _inverse[index] = true;
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
        _combines[_combineCount++] = {cb, mask, exclusive};
}

void NeuButton::refresh()
{
    _state = 0;
    for (uint8_t i = 0; i < _count; i++)
    {
        if ((digitalRead(_pins[i]) == LOW) ^ _inverse[i])
        {
            _state |= (1UL << i);
            _latchState |= (1UL << i);
        }
    }
    _lastState = _state;
}

void NeuButton::loop()
{
    uint32_t reading = 0;
    unsigned long now = millis();

    for (uint8_t i = 0; i < _count; i++)
    {
        if ((digitalRead(_pins[i]) == LOW) ^ _inverse[i])
            reading |= (1UL << i);
    }

    if (reading != _state)
    {
        _lastDebounceTime = now;
        _state = reading;
        return;
    }
    if ((now - _lastDebounceTime) < _debounceDelay)
        return;

    uint32_t changed = _state ^ _lastState;

    // 1. Transisi (Pressed / Released / Latch)
    if (changed)
    {
        for (uint8_t i = 0; i < _count; i++)
        {
            uint32_t mask = (1UL << i);
            if (changed & mask)
            {
                if (_state & mask)
                {
                    _pressTime[i] = now;
                    _lastRepeatTime[i] = now;
                    _longPresses[i].triggered = false;
                    if (_pressedCb)
                        _pressedCb(i);
                    _latchState ^= mask;
                    if (_latchCb)
                        _latchCb(i, (_latchState & mask) != 0);
                }
                else
                {
                    if (_releaseCb)
                        _releaseCb(i);
                }
            }
        }
        // 2. Kombinasi
        if (_state != 0)
        {
            for (uint8_t j = 0; j < _combineCount; j++)
            {
                bool match = (_state & _combines[j].mask) == _combines[j].mask;
                if (_combines[j].exclusive && _state != _combines[j].mask)
                    match = false;
                if (match && _combines[j].cb)
                    _combines[j].cb(_state);
            }
        }
    }

    // 3. Kondisi Ditahan (Long Press & Repeat)
    for (uint8_t i = 0; i < _count; i++)
    {
        if (_state & (1UL << i))
        {
            if (_longPresses[i].cb && !_longPresses[i].triggered)
            {
                if (now - _pressTime[i] >= _longPresses[i].time)
                {
                    _longPresses[i].cb(i);
                    _longPresses[i].triggered = true;
                }
            }
            if (_repeats[i].cb && (now - _lastRepeatTime[i] >= _repeats[i].time))
            {
                _repeats[i].cb(i);
                _lastRepeatTime[i] = now;
            }
        }
    }
    _lastState = _state;
}
