#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "TouchHandler.h"

#define TOUCH_IRQ  36
#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_CLK  25
#define TOUCH_CS   33

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

XPT2046_Touchscreen touchscreen(TOUCH_CS, TOUCH_IRQ);

void initTouch()
{
    SPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    touchscreen.begin();
    touchscreen.setRotation(1);
}

bool getTouchPoint(uint16_t &x, uint16_t &y, uint16_t &z)
{
    if (!(touchscreen.tirqTouched() && touchscreen.touched()))
    {
        return false;
    }

    TS_Point p = touchscreen.getPoint();

    // CYD working example ranges
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;

    // lihtne sanity check
    if (x > SCREEN_WIDTH || y > SCREEN_HEIGHT || z == 0)
    {
        return false;
    }

    return true;
}