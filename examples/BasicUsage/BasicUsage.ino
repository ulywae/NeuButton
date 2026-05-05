#include <NeuButton.h>

const uint8_t pins[] = {2};
NeuButton btn(pins, 1);

void setup()
{
    Serial.begin(115200);

    btn.onPressed([](uint8_t index)
                  { Serial.println("Button Clicked!"); });

    btn.onLatch([](uint8_t index, bool state)
                {
    Serial.print("Switch Mode: ");
    Serial.println(state ? "ON" : "OFF"); });

    btn.refresh();
}

void loop()
{
    btn.loop();
}
