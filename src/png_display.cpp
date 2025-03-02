#include "png_display.h"

static PNG png;

// File handling functions for PNGdec
void *pngOpen(const char *filename, int32_t *size) {
    File file = SPIFFS.open(filename, "r");
    if (!file) return nullptr;
    *size = file.size();
    return new File(file);
}

void pngClose(void *handle) {
    File *file = static_cast<File *>(handle);
    if (file) {
        file->close();
        delete file;
    }
}

int32_t pngRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
    File *file = reinterpret_cast<File *>(handle->fHandle);
    return file ? file->read(buffer, length) : 0;
}

int32_t pngSeek(PNGFILE *handle, int32_t position) {
    File *file = reinterpret_cast<File *>(handle->fHandle);
    return file ? (file->seek(position) ? position : -1) : -1;
}
// Callback function to draw PNG lines on TFT display
void pngDraw(PNGDRAW *pDraw) {
    uint16_t lineBuffer[pDraw->iWidth];
    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xFFFF);
    TFT_eSPI *tft = (TFT_eSPI *)pDraw->pUser;
    tft->pushImage(0, pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

// Function to load and display a PNG file from SPIFFS
void loadAndDisplayPNG(const char *filename, TFT_eSPI &tft) {
    if (png.open(filename, pngOpen, pngClose, pngRead, pngSeek, pngDraw) == PNG_SUCCESS) {

        png.decode(nullptr, 0);
        png.close();
    } else {
        Serial.println("PNG loading failed!");
    }
}
