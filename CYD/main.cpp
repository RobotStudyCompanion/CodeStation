#include <Arduino.h>
#include "AnimationPlayer.h"

void handleCommand(String cmd)
{
    cmd.trim();
    cmd.toLowerCase();

    Serial.print("Received: ");
    Serial.println(cmd);

    if (cmd == "caring")
    {
        switchFolder("/caring");
        Serial.println("Switched to /caring");
    }
    else if (cmd == "surprised")
    {
        switchFolder("/surprised");
        Serial.println("Switched to /surprised");
    }
    else if (cmd == "pride")
    {
        switchFolder("/pride");
        Serial.println("Switched to /pride");
    }
    else
    {
        Serial.println("Unknown command");
    }
}

void setup() {
    Serial.begin(115200);

    used_to_be_setup();
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