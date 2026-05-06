#include <NeuButton.h>

const uint8_t pins[] = {2, 3, 4};
const uint8_t numPins = sizeof(pins) / sizeof(pins[0]);

NeuButton btn(pins, numPins);

void setup()
{
    Serial.begin(115200);

    // Button 0 + Button 1
    btn.addCombine([](uint32_t mask) {
        Serial.println("Combo: 0 + 1");
    }, (1 << 0) | (1 << 1), true);

    // Button 1 + Button 2 (non-exclusive)
    btn.addCombine([](uint32_t mask) {
        Serial.println("Combo: 1 + 2 (non-exclusive)");
    }, (1 << 1) | (1 << 2), false);

    btn.onPressed([](uint8_t i) {
        Serial.print("Pressed: ");
        Serial.println(i);
    });

    btn.refresh();
}

void loop()
{
    btn.loop();
}