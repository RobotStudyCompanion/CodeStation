#include <Arduino.h>
#include "AnimationPlayer.h"
#include "LED_Solution.h"

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

void setup() {
    Serial.begin(115200);

    used_to_be_setup();//Edited setup from animationplayer.
    setupLed();
    switchFolder("/pride");
    

    Serial.println("Ready for UART commands");

}

void loop() {
    if (Serial.available()){
        String cmd = Serial.readStringUntil('\n');
        handleCommand(cmd);
    }

    updateAnimationPlayer();
}