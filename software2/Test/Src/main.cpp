#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN     PB9
#define NUMPIXELS   1

Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

/* ---------------------------------------------------------------
 * Predefined color names
 * --------------------------------------------------------------- */
struct ColorEntry {
    const char *name;
    uint8_t r, g, b;
};

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

/* Parse "R,G,B" format, e.g. "255,0,128" */
static bool parseRGB(const String &s, uint8_t &r, uint8_t &g, uint8_t &b)
{
    int c1 = s.indexOf(',');
    int c2 = s.lastIndexOf(',');
    if (c1 < 0 || c1 == c2) return false;
    r = (uint8_t)s.substring(0, c1).toInt();
    g = (uint8_t)s.substring(c1 + 1, c2).toInt();
    b = (uint8_t)s.substring(c2 + 1).toInt();
    return true;
}

/* Parse "#RRGGBB" or "RRGGBB" hex format */
static bool parseHex(const String &s, uint8_t &r, uint8_t &g, uint8_t &b)
{
    String h = s.startsWith("#") ? s.substring(1) : s;
    if (h.length() != 6) return false;
    long v = strtol(h.c_str(), nullptr, 16);
    r = (v >> 16) & 0xFF;
    g = (v >>  8) & 0xFF;
    b =  v        & 0xFF;
    return true;
}

static void handleCommand(String cmd)
{
    cmd.trim();
    cmd.toLowerCase();
    if (cmd.length() == 0) return;

    /* Try named colors */
    for (const auto &c : COLORS) {
        if (cmd == c.name) {
            setColor(c.r, c.g, c.b);
            SerialUSB.print("OK: "); SerialUSB.println(cmd);
            return;
        }
    }

    /* Try R,G,B */
    uint8_t r, g, b;
    if (parseRGB(cmd, r, g, b)) {
        setColor(r, g, b);
        SerialUSB.print("OK: rgb("); SerialUSB.print(r); SerialUSB.print(',');
        SerialUSB.print(g); SerialUSB.print(','); SerialUSB.print(b);
        SerialUSB.println(')');
        return;
    }

    /* Try #RRGGBB */
    if (parseHex(cmd, r, g, b)) {
        setColor(r, g, b);
        SerialUSB.print("OK: #"); SerialUSB.println(cmd.startsWith("#") ? cmd.substring(1) : cmd);
        return;
    }

    SerialUSB.println("ERR: unknown command");
    SerialUSB.println("  colors : red green blue yellow cyan magenta white orange off");
    SerialUSB.println("  rgb    : 255,0,128");
    SerialUSB.println("  hex    : #FF0080");
}

void setup()
{
    SerialUSB.begin();
    while (!SerialUSB);   /* wait for host to open the port */

    strip.begin();
    strip.setBrightness(128);
    strip.show();

    SerialUSB.println("NeoPixel USB CDC ready.");
    SerialUSB.println("Send a color name, R,G,B or #RRGGBB:");
}

void loop()
{
    static String buf;

    while (SerialUSB.available()) {
        char c = (char)SerialUSB.read();
        if (c == '\n' || c == '\r') {
            if (buf.length() > 0) {
                handleCommand(buf);
                buf = "";
            }
        } else {
            buf += c;
        }
    }
}
