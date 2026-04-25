#include <Arduino.h>

//pins for the RGB LED
const int LED_R = 4;
const int LED_G = 16;
const int LED_B = 17;

void setLedColor(bool redOn, bool greenOn, bool blueOn)
{
    // active-low LED:
    digitalWrite(LED_R, redOn ? LOW : HIGH);
    digitalWrite(LED_G, greenOn ? LOW : HIGH);
    digitalWrite(LED_B, blueOn ? LOW : HIGH);
}

void setupLed()
{
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);

    // alguses kõik välja
    setLedColor(false, false, false);
}