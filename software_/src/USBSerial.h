#ifndef USBSERIAL_H
#define USBSERIAL_H

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

extern "C" {
#include "usbdrv/usbdrv.h"
}

// CDC ACM specific defines
#define CDC_SET_LINE_CODING         0x20
#define CDC_GET_LINE_CODING         0x21
#define CDC_SET_CONTROL_LINE_STATE  0x22
#define CDC_SEND_BREAK             0x23

// Line coding structure for CDC ACM
typedef struct {
    uint32_t dwDTERate;   // baud rate
    uint8_t bCharFormat;  // stop bits
    uint8_t bParityType;  // parity
    uint8_t bDataBits;    // data bits
} cdc_line_coding_t;

class USBSerial {
public:
    USBSerial();
    
    // Initialize USB communication
    void begin();
    
    // Poll USB for incoming data (must be called frequently)
    void poll();
    
    // Read data from serial buffer
    // Returns number of bytes read, or 0 if no complete line available
    // maxLen should include space for null terminator
    int read(char* buffer, int maxLen);
    
    // Check if data is available
    bool available();
    
    // USB callback functions (need to be public for C linkage)
    static usbMsgLen_t handleSetup(uchar data[8]);
    static uchar handleWrite(uchar *data, uchar len);
    static void handleWriteOut(uchar *data, uchar len);
    static uchar handleRead(uchar *data, uchar len);
    static usbMsgLen_t handleDescriptor(struct usbRequest *rq);
    
private:
    static char serialBuffer[64];
    static uint8_t serialBufferPos;
    static cdc_line_coding_t lineCoding;
    static bool dataReady;
    static char completeLine[64];
    
    static void processReceivedData(char c);
};

// Global instance
extern USBSerial usbSerial;

#endif
