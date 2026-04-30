// AnimationPlayer.cpp
// Drives MJPEG animation playback on the Cheap Yellow Display (CYD) via an SD card.
// Adapted from: https://youtu.be/jYcxUgxz9ks
// Original code is NOT mine; edited to suit RSC (Robotic Study Companion) needs.
//
// Target board : "ESP32 Dev Module" (last tested with Espressif Arduino Core v3.2.0)
//
// How it works:
//   1. On first call to used_to_be_setup(), the display and SD card are initialised
//      and a list of .mjpeg files in the current emotion folder is built.
//   2. updateAnimationPlayer() is called every loop iteration.  It plays the next
//      .mjpeg file in the list and advances the index, looping back to 0 when it
//      reaches the end.
//   3. switchFolder() resets the index and rebuilds the file list for a new emotion
//      folder, allowing main.cpp to change the displayed emotion at any time.
//   4. The physical boot button (GPIO 0) triggers an interrupt that sets skipRequested,
//      causing the current animation to be cut short and the next one to start.

#include <Arduino_GFX_Library.h> // Install "GFX Library for Arduino" with the Library Manager (last tested on v1.6.0)
                                 // Install "JPEGDEC" with the Library Manager (last tested on v1.8.2)
#include "MjpegClass.h"          // Included in this project
#include "SD.h"                  // Included with the Espressif Arduino Core (last tested on v3.2.0)

// ---------------------------------------------------------------------------
// Pin definitions
// ---------------------------------------------------------------------------
// Display (ILI9341) is on the primary HSPI bus; SD card uses VSPI.
#define BL_PIN 21   // Display backlight enable. Some CYD variants use pin 27 instead.
#define SD_CS  5    // SD card chip-select
#define SD_MISO 19  // SD VSPI MISO
#define SD_MOSI 23  // SD VSPI MOSI
#define SD_SCK  18  // SD VSPI clock

#define BOOT_PIN 0                   // Boot / skip button (active LOW)
#define BOOT_BUTTON_DEBOUCE_TIME 400 // Minimum ms between recognised button presses

// ---------------------------------------------------------------------------
// SPI speed configuration
// ---------------------------------------------------------------------------
// Some CYD variants are unstable at 80 MHz; uncomment the 40 MHz line if needed.
// #define DISPLAY_SPI_SPEED 40000000L // 40 MHz
#define DISPLAY_SPI_SPEED 80000000L   // 80 MHz
#define SD_SPI_SPEED      80000000L   // 80 MHz

// ---------------------------------------------------------------------------
// Animation state
// ---------------------------------------------------------------------------
String currentFolder = "/mjpeg"; // Active emotion folder on the SD card

// In-memory list of .mjpeg filenames found in currentFolder
#define MAX_FILES 20
String   mjpegFileList[MAX_FILES];
uint32_t mjpegFileSizes[MAX_FILES] = {0}; // Size of each file in bytes
int      mjpegCount = 0;
static int  currentMjpegIndex = 0;
static File mjpegFile; // Temporary file handle used during playback

// ---------------------------------------------------------------------------
// MJPEG decoder state (allocated once in used_to_be_setup)
// ---------------------------------------------------------------------------
MjpegClass    mjpeg;
int           total_frames;
unsigned long total_read_video;
unsigned long total_decode_video;
unsigned long total_show_video;
unsigned long start_ms, curr_ms;
long          output_buf_size, estimateBufferSize;
uint8_t  *mjpeg_buf;   // JPEG bitstream buffer (PSRAM / 8-bit capable heap)
uint16_t *output_buf;  // Decoded pixel row buffer (DMA-capable, 16-bit aligned)

// ---------------------------------------------------------------------------
// Display driver
// ---------------------------------------------------------------------------
// The CYD uses an ILI9341 2.8" 240×320 display on the HSPI bus.
// Pin mapping: DC=2, CS=15, SCK=14, MOSI=13, MISO=12
Arduino_DataBus *bus = new Arduino_HWSPI(2 /* DC */, 15 /* CS */, 14 /* SCK */, 13 /* MOSI */, 12 /* MISO */);
Arduino_GFX     *gfx = new Arduino_ILI9341(bus);

// SD card reader is on the separate VSPI peripheral
SPIClass sd_spi(VSPI);

// ---------------------------------------------------------------------------
// Boot-button interrupt (runs in IRAM for safety)
// ---------------------------------------------------------------------------
volatile bool     skipRequested = false; // Set by ISR; cleared after the current animation ends
volatile uint32_t isrTick = 0;           // FreeRTOS tick captured at interrupt time
uint32_t          lastPress = 0;         // Used in main context for debounce

/**
 * @brief ISR fired on the FALLING edge of BOOT_PIN.
 *        Sets skipRequested so the main loop can abort the current animation.
 */
void IRAM_ATTR onButtonPress()
{
    skipRequested = true;
    isrTick = xTaskGetTickCountFromISR();
}

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
void   loadMjpegFilesList();
void   playSelectedMjpeg(int mjpegIndex);
void   mjpegPlayFromSDCard(char *mjpegFilename);
void   switchFolder(const String& newFolder);
String formatBytes(size_t bytes);


/**
 * @brief One-time hardware and buffer initialisation.
 *
 * Must be called once before the first call to updateAnimationPlayer().
 * Initialises:
 *  - Display backlight and ILI9341 driver
 *  - SD card (VSPI, mounted at "/sd")
 *  - DMA output buffer and MJPEG bitstream buffer
 *  - Boot-button interrupt
 *  - Initial file list for the default emotion folder
 */
void used_to_be_setup()
{
    // Turn on the display backlight
    pinMode(BL_PIN, OUTPUT);
    digitalWrite(BL_PIN, HIGH);

    // Initialise the ILI9341 display
    if (!gfx->begin(DISPLAY_SPI_SPEED))
    {
        // Display init failed – halt; check wiring and SPI speed
        while (true) { /* no need to continue */ }
    }
    gfx->setRotation(0);
    gfx->fillScreen(RGB565_BLACK);
    // gfx->invertDisplay(true); // Uncomment on CYD variants with inverted colour

    // Initialise the SD card on VSPI
    if (!SD.begin(SD_CS, sd_spi, SD_SPI_SPEED, "/sd"))
    {
        // SD init failed – halt; check card is inserted and wiring is correct
        while (true) { /* no need to continue */ }
    }

    // Allocate the DMA-capable output (pixel row) buffer
    output_buf_size = gfx->width() * 4 * 2;
    output_buf = (uint16_t *)heap_caps_aligned_alloc(16, output_buf_size * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!output_buf)
    {
        while (true) { /* DMA buffer allocation failed */ }
    }

    // Allocate the MJPEG bitstream buffer
    estimateBufferSize = gfx->width() * gfx->height() * 2 / 5;
    mjpeg_buf = (uint8_t *)heap_caps_malloc(estimateBufferSize, MALLOC_CAP_8BIT);
    if (!mjpeg_buf)
    {
        while (true) { /* MJPEG buffer allocation failed */ }
    }

    loadMjpegFilesList(); // Populate the file list for the initial folder

    // Attach the boot button interrupt (skip to next animation on press)
    pinMode(BOOT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(BOOT_PIN), onButtonPress, FALLING);
}


/**
 * @brief Advance and play the next animation in the current folder.
 *
 * Call this every loop() iteration.  It plays the .mjpeg at currentMjpegIndex,
 * then increments the index, wrapping back to 0 after the last file.
 */
void updateAnimationPlayer()
{
    playSelectedMjpeg(currentMjpegIndex);
    currentMjpegIndex++;

    if (currentMjpegIndex >= mjpegCount)
    {
        currentMjpegIndex = 0;
    }
}

/**
 * @brief Build the full SD path for a file index and start playback.
 * @param mjpegIndex Index into mjpegFileList[] for the file to play.
 */
void playSelectedMjpeg(int mjpegIndex)
{
    String fullPath = currentFolder + "/" + mjpegFileList[mjpegIndex];
    char mjpegFilename[128];
    fullPath.toCharArray(mjpegFilename, sizeof(mjpegFilename));

    mjpegPlayFromSDCard(mjpegFilename);
}

/**
 * @brief Switch the active emotion folder and reload the file list.
 *
 * After this call, updateAnimationPlayer() will play animations from newFolder,
 * starting from the first file.
 *
 * @param newFolder Absolute SD-card path to the emotion folder (e.g. "/caring").
 */
void switchFolder(const String& newFolder)
{
    currentFolder = newFolder;
    currentMjpegIndex = 0;
    loadMjpegFilesList();
}

/**
 * @brief JPEGDEC draw callback – blits a decoded JPEG tile to the display.
 *
 * Called repeatedly by the MJPEG decoder for each 16-pixel-high tile decoded
 * from a single video frame.
 *
 * @param pDraw Pointer to the JPEGDRAW struct provided by JPEGDEC.
 * @return 1 to continue decoding, 0 to abort.
 */
int jpegDrawCallback(JPEGDRAW *pDraw)
{
    unsigned long s = millis();
    gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
    total_show_video += millis() - s;
    return 1;
}

/**
 * @brief Open and play a single .mjpeg file from the SD card.
 *
 * Streams the file frame-by-frame through the JPEGDEC decoder and draws each
 * frame to the display.  Playback ends when either:
 *   - The file has been fully read, or
 *   - The boot button sets skipRequested (with debounce handling).
 *
 * @param mjpegFilename Absolute SD path to the .mjpeg file (e.g. "/caring/idle.mjpeg").
 */
void mjpegPlayFromSDCard(char *mjpegFilename)
{
    File mjpegFile = SD.open(mjpegFilename, "r");

    if (!mjpegFile || mjpegFile.isDirectory())
    {
        // File could not be opened – skip silently and continue
        return;
    }

    gfx->fillScreen(RGB565_BLACK);

    start_ms = millis();
    curr_ms  = millis();
    total_frames       = 0;
    total_read_video   = 0;
    total_decode_video = 0;
    total_show_video   = 0;

    mjpeg.setup(
        &mjpegFile, mjpeg_buf, jpegDrawCallback, true /* useBigEndian */,
        0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);

    // Main decode/display loop – exits when the file ends or skip is requested
    while (!skipRequested && mjpegFile.available() && mjpeg.readMjpegBuf())
    {
        total_read_video += millis() - curr_ms;
        curr_ms = millis();

        mjpeg.drawJpg();
        total_decode_video += millis() - curr_ms;

        curr_ms = millis();
        total_frames++;
    }

    // Handle debounced skip-button press
    if (skipRequested)
    {
        uint32_t now = millis();
        if (now - lastPress >= BOOT_BUTTON_DEBOUCE_TIME)
        {
            lastPress = now;
        }
    }
    skipRequested = false; // Clear flag so the next animation plays normally

    mjpegFile.close();
}

/**
 * @brief Scan the current emotion folder and populate mjpegFileList[].
 *
 * Only files whose names end with ".mjpeg" are included.  Up to MAX_FILES
 * entries are stored; additional files beyond that limit are ignored.
 * Sets mjpegCount to the number of valid files found.
 */
void loadMjpegFilesList()
{
    File mjpegDir = SD.open(currentFolder);
    if (!mjpegDir)
    {
        // Folder not found – halt; check SD card contents
        while (true) { /* no need to continue */ }
    }

    mjpegCount = 0;
    while (true)
    {
        File file = mjpegDir.openNextFile();
        if (!file)
            break;

        if (!file.isDirectory())
        {
            String name = file.name();
            if (name.endsWith(".mjpeg"))
            {
                mjpegFileList[mjpegCount]  = name;
                mjpegFileSizes[mjpegCount] = file.size();
                mjpegCount++;
                if (mjpegCount >= MAX_FILES)
                    break;
            }
        }
        file.close();
    }
    mjpegDir.close();
}

/**
 * @brief Format a byte count as a human-readable string (B / KB / MB).
 * @param bytes Number of bytes to format.
 * @return Formatted string, e.g. "1.23 MB".
 */
String formatBytes(size_t bytes)
{
    if (bytes < 1024)
    {
        return String(bytes) + " B";
    }
    else if (bytes < (1024 * 1024))
    {
        return String(bytes / 1024.0, 2) + " KB";
    }
    else
    {
        return String(bytes / 1024.0 / 1024.0, 2) + " MB";
    }
}