#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "usb_msc_storage.h"

#define LED_PIN    PB9
#define NUMPIXELS  1

Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

/* ---------------------------------------------------------------
 * Predefined color names
 * --------------------------------------------------------------- */
struct ColorEntry { const char *name; uint8_t r, g, b; };

static const ColorEntry COLORS[] = {
    { "red",     255,   0,   0 },
    { "green",     0, 255,   0 },
    { "blue",      0,   0, 255 },
    { "yellow",  255, 255,   0 },
    { "cyan",      0, 255, 255 },
    { "magenta", 255,   0, 255 },
    { "white",   255, 255, 255 },
    { "orange",  255, 128,   0 },
    { "off",       0,   0,   0 },
};

static void setColor(uint8_t r, uint8_t g, uint8_t b)
{
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
}

/* Parse "R,G,B" */
static bool parseRGB(const char *s, uint8_t &r, uint8_t &g, uint8_t &b)
{
    int ri, gi, bi;
    if (sscanf(s, "%d,%d,%d", &ri, &gi, &bi) == 3) {
        r = (uint8_t)ri; g = (uint8_t)gi; b = (uint8_t)bi;
        return true;
    }
    return false;
}

/* Parse "#RRGGBB" or "RRGGBB" */
static bool parseHex(const char *s, uint8_t &r, uint8_t &g, uint8_t &b)
{
    if (*s == '#') s++;
    if (strlen(s) != 6) return false;
    long v = strtol(s, nullptr, 16);
    r = (v >> 16) & 0xFF;
    g = (v >>  8) & 0xFF;
    b =  v        & 0xFF;
    return true;
}

static void handleColor(const char *cmd)
{
    for (const auto &c : COLORS) {
        if (strcmp(cmd, c.name) == 0) { setColor(c.r, c.g, c.b); return; }
    }
    uint8_t r, g, b;
    if (parseRGB(cmd, r, g, b) || parseHex(cmd, r, g, b)) {
        setColor(r, g, b);
    }
}

void setup()
{
    strip.begin();
    strip.setBrightness(128);
    strip.show();

    MSC_init();   /* mount the virtual FAT12 disk over USB */
}

void loop()
{
    const char *color = MSC_get_pending_color();
    if (color) {
        handleColor(color);
    }
}
