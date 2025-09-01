#include <Adafruit_NeoPixel.h>

#include <DigisparkCDC/DigiCDC.h>

#define LED_PIN PB2
#define NUMPIXELS 1

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Variables for LED control
static uchar ledRed = 0, ledGreen = 20, ledBlue = 0; // Start with dim green for CDC mode

static char buffer[64];
static unsigned int bufferIndex = 0;
static unsigned long lastSerialTime = 0;

void setup()
{
	// Initialize NeoPixel first
	pixels.begin();
	pixels.setPixelColor(0, pixels.Color(0, 50, 0)); // Brighter green to indicate CDC mode
	pixels.show();

	SerialUSB.begin();
}

void loop()
{

	// USB polling - must be called frequently for enumeration
	SerialUSB.refresh();
	SerialUSB.write('\x06'); // Caractère ACK

	// Check if serial data is available
	if (SerialUSB.available())
	{

		int c = SerialUSB.read();
		if (c >= 0)
		{

			if (c == '#' || bufferIndex > sizeof(buffer))
			{
				bufferIndex = 0; // Reset buffer on '#' character
			}

			if ((c >= 0x30 && c <= 0x39) || // Si c'est un chiffre
				(c >= 0x41 && c <= 0x46) || // Si c'est une lettre majuscule hexadécimale
				(c >= 0x61 && c <= 0x66))	// Si c'est une lettre minuscule hexadécimale
			{
				buffer[bufferIndex++] = (char)c;
			}
			else
			{
				bufferIndex = 0;
			}

			if (bufferIndex >= 6)
			{
				buffer[bufferIndex] = '\0'; // Null-terminate the string

				unsigned long color = strtol(buffer, NULL, 16);

				ledRed = (color >> 16) & 0xFF;	// Extract red component
				ledGreen = (color >> 8) & 0xFF; // Extract green component
				ledBlue = color & 0xFF;			// Extract blue component

				// Reset buffer after processing
				bufferIndex = 0;
			}
		}
	}

	SerialUSB.clearWriteError();

	// Update LED color
	pixels.setPixelColor(0, pixels.Color(ledRed, ledGreen, ledBlue));
	pixels.show();
}