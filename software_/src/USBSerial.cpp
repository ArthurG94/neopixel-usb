#include "USBSerial.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

// CDC ACM configuration descriptor
const char cdcConfigDescriptor[] PROGMEM = {
    // Configuration descriptor
    9,          // bLength
    2,          // bDescriptorType (Configuration)
    67, 0,      // wTotalLength (67 bytes)
    2,          // bNumInterfaces
    1,          // bConfigurationValue
    0,          // iConfiguration
    (char)0x80, // bmAttributes (bus powered)
    50,         // bMaxPower (100mA)

    // Interface 0: Communication Interface
    9,          // bLength
    4,          // bDescriptorType (Interface)
    0,          // bInterfaceNumber
    0,          // bAlternateSetting
    1,          // bNumEndpoints
    0x02,       // bInterfaceClass (CDC)
    0x02,       // bInterfaceSubClass (ACM)
    0x01,       // bInterfaceProtocol (AT Commands)
    0,          // iInterface

    // CDC Header Functional Descriptor
    5,          // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x00,       // bDescriptorSubtype (Header)
    0x10, 0x01, // bcdCDC (1.10)

    // CDC Call Management Functional Descriptor
    5,          // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x01,       // bDescriptorSubtype (Call Management)
    0x00,       // bmCapabilities (no call management)
    1,          // bDataInterface

    // CDC ACM Functional Descriptor
    4,          // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x02,       // bDescriptorSubtype (ACM)
    0x02,       // bmCapabilities (line coding and serial state)

    // CDC Union Functional Descriptor
    5,          // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x06,       // bDescriptorSubtype (Union)
    0,          // bMasterInterface (interface 0)
    1,          // bSlaveInterface (interface 1)

    // Endpoint 1 IN (notification endpoint)
    7,          // bLength
    5,          // bDescriptorType (Endpoint)
    (char)0x81, // bEndpointAddress (IN, endpoint 1)
    0x03,       // bmAttributes (interrupt)
    8, 0,       // wMaxPacketSize (8 bytes)
    10,         // bInterval (10ms)

    // Interface 1: Data Interface
    9,          // bLength
    4,          // bDescriptorType (Interface)
    1,          // bInterfaceNumber
    0,          // bAlternateSetting
    2,          // bNumEndpoints
    0x0A,       // bInterfaceClass (CDC Data)
    0x00,       // bInterfaceSubClass
    0x00,       // bInterfaceProtocol
    0,          // iInterface

    // Endpoint 3 OUT (data out)
    7,          // bLength
    5,          // bDescriptorType (Endpoint)
    0x03,       // bEndpointAddress (OUT, endpoint 3)
    0x02,       // bmAttributes (bulk)
    8, 0,       // wMaxPacketSize (8 bytes)
    0,          // bInterval (ignored for bulk)

    // Endpoint 3 IN (data in)
    7,          // bLength
    5,          // bDescriptorType (Endpoint)
    (char)0x83, // bEndpointAddress (IN, endpoint 3)
    0x02,       // bmAttributes (bulk)
    8, 0,       // wMaxPacketSize (8 bytes)
    0           // bInterval (ignored for bulk)
};

// Static member definitions
char USBSerial::serialBuffer[64];
uint8_t USBSerial::serialBufferPos = 0;
cdc_line_coding_t USBSerial::lineCoding = {9600, 0, 0, 8};
bool USBSerial::dataReady = false;
char USBSerial::completeLine[64];

// Global instance
USBSerial usbSerial;

USBSerial::USBSerial() {
    serialBufferPos = 0;
    dataReady = false;
    memset(serialBuffer, 0, sizeof(serialBuffer));
    memset(completeLine, 0, sizeof(completeLine));
}

void USBSerial::begin() {
    // Disable watchdog if enabled by bootloader/fuses
    wdt_disable();
    
    // Disable interrupts during initialization
    cli();
    
    // Initialize USB
    usbInit();
    
    // Force USB disconnect for clean re-enumeration
    usbDeviceDisconnect();
    
    // Wait at least 500ms for host to detect disconnect
    // Use longer delay for better compatibility
    for(uchar i = 0; i < 50; i++) {
        _delay_ms(10);
    }
    
    // Clear any pending interrupts before enabling
    GIFR |= (1 << PCIF);
    
    // Reconnect USB
    usbDeviceConnect();
    
    // Enable interrupts
    sei();
    
    // Clear serial buffer
    serialBufferPos = 0;
    dataReady = false;
}

int USBSerial::read(char* buffer, int maxLen) {
    if (!dataReady || maxLen <= 0) {
        return 0;
    }
    
    int len = strlen(completeLine);
    if (len >= maxLen) {
        len = maxLen - 1; // Leave space for null terminator
    }
    
    memcpy(buffer, completeLine, len);
    buffer[len] = '\0';
    
    dataReady = false; // Mark as consumed
    return len;
}


void USBSerial::poll() {
    usbPoll();
}

bool USBSerial::available() {
    return dataReady;
}

void USBSerial::processReceivedData(char c) {
    if (c == '\n' || c == '\r') {
        // End of line - complete command available
        if (serialBufferPos > 0) {
            serialBuffer[serialBufferPos] = '\0';
            strcpy(completeLine, serialBuffer);
            dataReady = true;
            serialBufferPos = 0;
        }
    } else if (serialBufferPos < sizeof(serialBuffer) - 1) {
        // Add character to buffer
        serialBuffer[serialBufferPos++] = c;
    }
}

// Static callback functions for C linkage
usbMsgLen_t USBSerial::handleDescriptor(struct usbRequest *rq) {
    if (rq->wValue.bytes[1] == USBDESCR_CONFIG) {
        usbMsgPtr = (uchar*)cdcConfigDescriptor;
        return sizeof(cdcConfigDescriptor);
    }
    return 0;
}

usbMsgLen_t USBSerial::handleSetup(uchar data[8]) {
    usbRequest_t *rq = (usbRequest_t *)data;
    
    // Handle CDC ACM specific requests
    if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        switch (rq->bRequest) {
            case CDC_SET_LINE_CODING:
                // Host wants to set line coding - we'll receive it in usbFunctionWrite
                return USB_NO_MSG;
                
            case CDC_GET_LINE_CODING:
                // Host wants to get line coding
                usbMsgPtr = (uchar*)&lineCoding;
                return sizeof(lineCoding);
                
            case CDC_SET_CONTROL_LINE_STATE:
                // Host is setting DTR/RTS state - just acknowledge
                return 0;
                
            case CDC_SEND_BREAK:
                // Host wants to send a break - just acknowledge
                return 0;
        }
    }
    
    return 0; // Unknown request
}

uchar USBSerial::handleWrite(uchar *data, uchar len) {
    static uchar bytesRemaining = 0;
    static uchar* currentPtr = NULL;
    
    // Handle line coding setup
    if (bytesRemaining == 0) {
        // This could be line coding data (7 bytes) or regular data
        if (len >= sizeof(lineCoding)) {
            // Assume this is line coding
            memcpy(&lineCoding, data, sizeof(lineCoding));
            return 1; // Data processed
        } else {
            // This is regular serial data
            bytesRemaining = len;
            currentPtr = data;
        }
    }
    
    // Process serial data
    while (bytesRemaining > 0) {
        char c = *currentPtr++;
        bytesRemaining--;
        processReceivedData(c);
    }
    
    return 1; // Data processed
}

void USBSerial::handleWriteOut(uchar *data, uchar len) {
    // Process received CDC data
    for (uchar i = 0; i < len; i++) {
        char c = data[i];
        processReceivedData(c);
    }
}

uchar USBSerial::handleRead(uchar *data, uchar len) {
    // For now, we don't send data back to the host
    // This would be used to send responses or status information
    return 0;
}

// C wrapper functions for USB callbacks
extern "C" usbMsgLen_t usbFunctionDescriptor(struct usbRequest *rq) {
    return USBSerial::handleDescriptor(rq);
}

extern "C" usbMsgLen_t usbFunctionSetup(uchar data[8]) {
    return USBSerial::handleSetup(data);
}

extern "C" uchar usbFunctionWrite(uchar *data, uchar len) {
    return USBSerial::handleWrite(data, len);
}

extern "C" void usbFunctionWriteOut(uchar *data, uchar len) {
    USBSerial::handleWriteOut(data, len);
}

extern "C" uchar usbFunctionRead(uchar *data, uchar len) {
    return USBSerial::handleRead(data, len);
}
