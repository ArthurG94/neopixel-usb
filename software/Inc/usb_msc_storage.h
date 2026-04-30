#pragma once
#ifdef USBCON

#ifdef __cplusplus
extern "C" {
#endif

/** Initialise the USB MSC device (virtual FAT12 disk). Call once in setup(). */
void MSC_init(void);

/**
 * Returns the color string written to color.txt by the host,
 * or nullptr if the file has not been modified since the last call.
 * The string is lowercase and null-terminated.
 */
const char *MSC_get_pending_color(void);

#ifdef __cplusplus
}
#endif

#endif /* USBCON */
