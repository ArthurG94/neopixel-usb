/**
 * Custom USB endpoint / PMA configuration for MSC-only device.
 *
 * Shadows the framework's USBDevice/inc/usbd_ep_conf.h.
 * ep_def[] is defined in Src/usbd_ep_conf.c.
 *
 * PMA layout (STM32F072 USB-FS, 512 B PMA):
 *   [  0 – 23 ]  Buffer Descriptor Table  (3 × 8 B)
 *   [ 24 – 87 ]  EP0 OUT data  (64 B)
 *   [ 88 –151 ]  EP0 IN  data  (64 B)
 *   [152 –215 ]  EP1 OUT data  (64 B)   ← MSC Bulk OUT
 *   [216 –279 ]  EP1 IN  data  (64 B)   ← MSC Bulk IN
 */

#ifndef __USBD_EP_CONF_H
#define __USBD_EP_CONF_H

#ifdef USBCON

#include <stdint.h>
#include "usbd_def.h"

typedef struct {
  uint32_t ep_adress;
  uint32_t ep_size;
#if defined(USB)
  uint32_t ep_kind;   /* PCD_SNG_BUF or PCD_DBL_BUF */
#endif
} ep_desc_t;

/* MSC bulk endpoints --------------------------------------------------------*/
#define MSC_EP_SIZE       0x40U   /* 64 bytes (FS full-speed packet)   */

/* HID defines kept for usbd_hid_composite.c (compiled by framework even
 * though HID is never initialised at runtime in this project).          */
#define HID_MOUSE_EPIN_ADDR       0x81U
#define HID_KEYBOARD_EPIN_ADDR    0x82U
#define HID_MOUSE_EPIN_SIZE       0x04U
#define HID_KEYBOARD_EPIN_SIZE    0x08U

/*
 * DEV_NUM_EP: ep_def[] has (DEV_NUM_EP + 1) entries.
 * MSC needs 4 entries: EP0_OUT, EP0_IN, EP1_OUT(0x01), EP1_IN(0x81)
 *   → DEV_NUM_EP = 3
 * BDT = 8 * 3 = 24 bytes (slots for EP0, EP1, EP2; EP2 unused but harmless).
 */
#define DEV_NUM_EP        0x03U

#if defined(USB)  /* STM32F0/F1/F3 USB FS peripheral (not OTG) */
#define PMA_EP0_OUT_ADDR   (8U * DEV_NUM_EP)                         /* 0x18 = 24  */
#define PMA_EP0_IN_ADDR    (PMA_EP0_OUT_ADDR + USB_MAX_EP0_SIZE)     /* 0x58 = 88  */
#define PMA_MSC_OUT_ADDR   (PMA_EP0_IN_ADDR  + USB_MAX_EP0_SIZE)     /* 0x98 = 152 */
#define PMA_MSC_IN_ADDR    (PMA_MSC_OUT_ADDR + MSC_EP_SIZE)          /* 0xD8 = 216 */
#endif /* USB */

extern const ep_desc_t ep_def[DEV_NUM_EP + 1];

#endif /* USBCON */
#endif /* __USBD_EP_CONF_H */
