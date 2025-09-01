#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <string.h>
#include "USBSerial.h"

#ifdef __AVR__
  #include <avr/power.h>
#endif

#define LED_PIN        PB2
#define NUMPIXELS 1

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Variables for LED control
static uchar ledRed = 0, ledGreen = 20, ledBlue = 0;  // Start with dim green for CDC mode

// Parse RGB command from serial input
void parseRGBCommand(const char* cmd) {
    // Expect format: "RGB:255,128,64\n" or "RGB:255,128,64\r\n"
    if (strncmp(cmd, "RGB:", 4) == 0) {
        int r, g, b;
        if (sscanf(cmd + 4, "%d,%d,%d", &r, &g, &b) == 3) {
            ledRed = (r < 0) ? 0 : (r > 255) ? 255 : r;
            ledGreen = (g < 0) ? 0 : (g > 255) ? 255 : g;
            ledBlue = (b < 0) ? 0 : (b > 255) ? 255 : b;
            
            // Update LED color
            pixels.setPixelColor(0, pixels.Color(ledRed, ledGreen, ledBlue));
            pixels.show();
        }
    }
}

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  // Disable watchdog if enabled by bootloader/fuses
  wdt_disable();
  
  // Initialize pixels
  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(0, 0, 10)); // Dim blue to indicate startup
  pixels.show();
  
  // Initialize USB communication
  usbSerial.begin();
  
  // Clear LED after initialization
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();
}

void loop() {
  // USB polling - must be called frequently
  usbSerial.poll();
  
  // Check if serial data is available
  if (usbSerial.available()) {
    char buffer[64];
    int len = usbSerial.read(buffer, sizeof(buffer));
    if (len > 0) {
      parseRGBCommand(buffer);
    }
  }
  
  // Small delay to avoid overwhelming the USB stack
  _delay_ms(1);
}