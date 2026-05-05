#ifndef NEUBUTTON_H
#define NEUBUTTON_H

#include <Arduino.h>

typedef void (*ButtonCallback)(uint8_t index);
typedef void (*CombineCallback)(uint32_t mask);
typedef void (*LatchCallback)(uint8_t index, bool latched);

class NeuButton
{
public:
    NeuButton(const uint8_t *pins, uint8_t count);
    ~NeuButton();

    // Event Registration
    void onPressed(ButtonCallback cb);
    void onRelease(ButtonCallback cb);
    void onLatch(LatchCallback cb);
    void addCombine(CombineCallback cb, uint32_t mask, bool exclusive = false);
    void addRepeat(uint8_t index, uint32_t interval, ButtonCallback cb);
    void addLongPress(uint8_t index, uint32_t duration, ButtonCallback cb);

    // Getters (Untuk cek status kapan saja)
    bool isPressed(uint8_t index);
    bool isLatched(uint8_t index);
    uint32_t getRawState();

    void setInverse(uint8_t index);
    void refresh();
    void loop();

private:
    struct CombineEntry
    {
        CombineCallback cb;
        uint32_t mask;
        bool exclusive;
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
    uint32_t _state, _lastState, _latchState;

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
