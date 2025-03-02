#ifndef PNG_DISPLAY_H
#define PNG_DISPLAY_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <PNGdec.h>
#include <TFT_eSPI.h>

// Function to load and display a PNG file
void loadAndDisplayPNG(const char *filename, TFT_eSPI &tft);

#endif // PNG_DISPLAY_H
