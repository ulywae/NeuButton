#include <NeuButton.h>

const uint8_t pins[] = {2, 3};
NeuButton<2> btn(pins);

void setup()
{
    Serial.begin(115200);

    // Button 0: Hold for 3 seconds to trigger
    btn.addLongPress(0, 3000, [](uint8_t index)
                     { Serial.println("System Reset Triggered!"); });

    // Button 1: Increase value while holding
    btn.addRepeat(1, 150, [](uint8_t index)
                  {
    static int value = 0;
    value++;
    Serial.print("Value: ");
    Serial.println(value); });

    btn.refresh();
}

void loop()
{
    btn.loop();
}
