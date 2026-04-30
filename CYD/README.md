# CYD – Cheap Yellow Display Emotion Player

This sub-project runs on the **Cheap Yellow Display (CYD)** — an ESP32 development board with a built-in 2.8 inch ILI9341 TFT display and an SD card slot.  It receives plain-text emotion commands from the RSC Raspberry Pi over hardware UART and plays the matching MJPEG animation loop on the screen.

---

## Hardware

| Component | Details |
|-----------|---------|
| MCU | ESP32 (Dual-core Xtensa LX6, 240 MHz) |
| Display | ILI9341 2.8" TFT, 240 × 320 px, 16-bit colour |
| Storage | microSD card (FAT32, connected via VSPI) |
| Interface | Hardware UART to Raspberry Pi (TX/RX) |
| Skip button | Boot button on GPIO 0 |

### Pin mapping

#### Display (HSPI)
| Signal | GPIO |
|--------|------|
| DC | 2 |
| CS | 15 |
| SCK | 14 |
| MOSI | 13 |
| MISO | 12 |
| Backlight | 21 *(some variants use 27)* |

#### SD card (VSPI)
| Signal | GPIO |
|--------|------|
| CS | 5 |
| MISO | 19 |
| MOSI | 23 |
| SCK | 18 |

---

## How it works

1. On boot, `setup()` initialises the display, SD card, and memory buffers, then switches to the `/pride` folder.
2. `loop()` listens for UART commands and calls `updateAnimationPlayer()` each iteration.
3. `updateAnimationPlayer()` plays the next `.mjpeg` file in the active folder and advances the index, looping continuously.
4. A UART command switches the active folder and restarts the file index so the new emotion begins immediately after the current animation ends.
5. Pressing the physical boot button (GPIO 0) skips the current animation and starts the next one.

### UART command protocol

| Command | Folder switched to | Emotion shown |
|---------|--------------------|---------------|
| `caring` | `/caring` | Caring / nurturing |
| `surprised` | `/surprised` | Surprised |
| `pride` | `/pride` | Pride / happy |

Commands are plain ASCII strings terminated with a newline (`\n`).  They are **case-insensitive** — leading and trailing whitespace is ignored.  Any unrecognised command is echoed back on Serial with `"Unknown command"`.

**Example** (sent from the Raspberry Pi):
```
echo "caring" > /dev/ttyUSB0
```

---

## SD card layout

Format the card as **FAT32** and create one folder per emotion.  Each folder contains `.mjpeg` animation files.  Up to **20 files** per folder are supported (configurable via `MAX_FILES` in `AnimationPlayer.cpp`).

```
SD card root/
├── caring/
│   ├── idle.mjpeg
│   └── loop.mjpeg
├── surprised/
│   └── surprise.mjpeg
└── pride/
    ├── happy01.mjpeg
    └── happy02.mjpeg
```

> **Tip:** Convert GIF or video clips to MJPEG using FFmpeg:
> ```bash
> ffmpeg -i input.gif -vf "scale=240:320" -q:v 5 output.mjpeg
> ```

---

## Dependencies & toolchain

| Library | Source | Tested version |
|---------|--------|----------------|
| Arduino core for ESP32 (Espressif) | [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32) | v3.2.0 |
| GFX Library for Arduino | Arduino Library Manager (`Arduino_GFX_Library`) | v1.6.0 |
| JPEGDEC | Arduino Library Manager (`JPEGDEC`) | v1.8.2 |
| SD (built-in) | Included with Espressif Arduino Core | v3.2.0 |
| MjpegClass | Included in this project (`MjpegClass.h`) | — |

---

## Building & flashing

1. Install [Arduino IDE 2](https://www.arduino.cc/en/software) or [PlatformIO](https://platformio.org/).
2. Add the Espressif ESP32 board package (URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`).
3. Select board **"ESP32 Dev Module"**.
4. Install the libraries listed above via the Library Manager.
5. Open `main.cpp` (or the `.ino` wrapper if using Arduino IDE), compile, and upload.
6. Open the Serial Monitor at **115200 baud** — you should see `Ready for UART commands` after boot.

---

## File structure

```
CYD/
├── AnimationPlayer.cpp   # Display driver, SD reader, MJPEG playback engine
├── AnimationPlayer.h     # Public API: used_to_be_setup(), updateAnimationPlayer(), switchFolder()
├── MjpegClass.h          # Lightweight MJPEG streaming helper (included in project)
├── main.cpp              # Arduino setup()/loop() + UART command handler
└── README.md             # This file
```

---

## Credits

The MJPEG playback core is based on the tutorial by **atomic14**:
- Video: <https://youtu.be/jYcxUgxz9ks>

Code has been edited to suit the RSC project's needs (UART command interface, folder-based emotion switching, boot-button skip).
