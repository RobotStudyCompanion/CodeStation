// main.cpp
// Entry point for the CYD (Cheap Yellow Display) emotion animation player.
//
// The RSC (Robotic Study Companion) Raspberry Pi sends plain-text emotion
// commands over the hardware UART (115200 baud, 8N1).  This sketch reads
// those commands and switches the MJPEG animation folder accordingly, so the
// CYD's display always reflects the robot's current emotional state.
//
// Supported UART commands (case-insensitive, terminated with '\n'):
//   caring    – play animations from /caring on the SD card
//   surprised – play animations from /surprised on the SD card
//   pride     – play animations from /pride on the SD card
//
// Any unrecognised command is logged to Serial and ignored.

#include <Arduino.h>
#include "AnimationPlayer.h"
#include "LED_Solution.h"
#include "TouchHandler.h"

/**
 * @brief Parse and dispatch a single UART command string.
 *
 * Leading/trailing whitespace is removed and the command is lowercased
 * before matching, so "  Caring\r\n" is treated the same as "caring".
 *
 * @param cmd Raw string read from Serial (including any trailing newline).
 */
void handleCommand(String cmd)
{
    cmd.trim();
    cmd.toLowerCase();

    Serial.print("Received: ");
    Serial.println(cmd);

    if (cmd == "caring")
    {
        switchFolder("/caring");
        setLedColor(true, false, false);
        Serial.println("Switched to /caring");
    }
    else if (cmd == "surprised")
    {
        switchFolder("/surprised");
        setLedColor(false, false, true);
        Serial.println("Switched to /surprised");
    }
    else if (cmd == "pride")
    {
        switchFolder("/pride");
        setLedColor(false, true, false);
        Serial.println("Switched to /pride");
    }
    else
    {
        Serial.println("Unknown command");
    }
}

/**
 * @brief Arduino setup – runs once after power-on or reset.
 *
 * Initialises Serial at 115200 baud, sets up the display and SD card via
 * AnimationPlayer, then switches to the default "/pride" folder so the
 * display is active immediately.
 */
void setup() {
    Serial.begin(115200);

    used_to_be_setup();       // Initialise display, SD card, and buffers
    initTouch();
    setupLed();
    switchFolder("/pride");   // Default emotion shown at startup
    

    Serial.println("Ready for UART commands");
}

bool touchWasDown = false;

/**
 * @brief Arduino main loop – runs repeatedly.
 *
 * Checks for an incoming UART command and dispatches it, then advances
 * the animation player by one frame/file.  Because mjpegPlayFromSDCard()
 * is blocking, a new command received while an animation is playing will
 * only be processed after the current animation finishes.
 */
void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        handleCommand(cmd);
    }

    updateAnimationPlayer();

    uint16_t x, y, z;
    bool touchNow = getTouchPoint(x, y, z);

    if (touchNow && !touchWasDown)
    {
        Serial.print("X = ");
        Serial.print(x);
        Serial.print(" | Y = ");
        Serial.print(y);
        Serial.print(" | Z = ");
        Serial.println(z);
    }

    touchWasDown = touchNow;
}