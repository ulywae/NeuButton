#include <NeuButton.h>

// Scenario: A 3-button controller (Pins 2, 3, 4)
const uint8_t pins[] = {2, 3, 4};
NeuButton<3> btn(pins);

void setup()
{
    Serial.begin(115200);

    // Combination: Press Button 0 + 2 (Mask: 5)
    // Exclusive = true means ONLY 0 and 2 must be pressed
    btn.addCombine([](uint32_t mask)
                   { Serial.println("SECRET COMBO: Extra Life Unlocked!"); }, 5, true);

    btn.onPressed([](uint8_t index)
                  {
    Serial.print("Action: Button ");
    Serial.println(index); });

    btn.refresh();
}

void loop()
{
    btn.loop();

    // Polling example: Check status manually every 5 seconds
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck > 5000)
    {
        if (btn.isLatched(1))
        {
            Serial.println("[LOG] Mode A is currently active.");
        }
        lastCheck = millis();
    }
}
