#include <NeuButton.h>

const uint8_t pins[] = {2, 3, 4};
const uint8_t numPins = sizeof(pins) / sizeof(pins[0]);

NeuButton btn(pins, numPins);

void setup()
{
    Serial.begin(115200);

    // Enable exclusive latch (radio-style selector)
    btn.setExclusiveLatch(true);

    btn.onLatch([](uint8_t i, bool state) {
        if (state)
        {
            Serial.print("Selected Mode: ");
            Serial.println(i);
        }
    });

    // Sync initial state (important for selector switches)
    btn.refresh();
}

void loop()
{
    btn.loop();
}