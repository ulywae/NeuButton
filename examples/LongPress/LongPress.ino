#include <NeuButton.h>

const uint8_t pins[] = {2, 3};
const uint8_t numPins = sizeof(pins) / sizeof(pins[0]);

NeuButton btn(pins, numPins);

void setup()
{
    Serial.begin(115200);

    // Long press on button 0 (2 seconds)
    btn.addLongPress(0, 2000, [](uint8_t i) {
        Serial.println("Button 0 LONG PRESS!");
    });

    // Repeat on button 1 (every 300ms)
    btn.addRepeat(1, 300, [](uint8_t i) {
        Serial.println("Button 1 REPEATING...");
    });

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