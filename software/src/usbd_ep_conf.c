/**
 * USB endpoint PMA configuration for MSC-only device.
 *
 * Replaces the framework's USBDevice/src/usbd_ep_conf.c.
 * The framework's copy is excluded by add_msc.py (AddBuildMiddleware).
 */
#if defined(HAL_PCD_MODULE_ENABLED) && defined(USBCON)

#include "usbd_ep_conf.h"

/* ep_def[] is iterated by USBD_LL_Init() to call HAL_PCDEx_PMAConfig()
 * for every endpoint before USB enumeration starts.                     */
const ep_desc_t ep_def[DEV_NUM_EP + 1] = {
#if defined(USB)   /* STM32F0/F1/F3 USB FS peripheral (not OTG) */
  /* EP0 control (bidirectional) */
  {0x00U, PMA_EP0_OUT_ADDR, PCD_SNG_BUF},
  {0x80U, PMA_EP0_IN_ADDR,  PCD_SNG_BUF},
  /* EP1 MSC Bulk OUT (host → device) */
  {0x01U, PMA_MSC_OUT_ADDR, PCD_SNG_BUF},
  /* EP1 MSC Bulk IN  (device → host) */
  {0x81U, PMA_MSC_IN_ADDR,  PCD_SNG_BUF},
#else
  /* OTG-FS / OTG-HS: use FIFO sizes instead of PMA addresses */
  {0x00U, USB_FS_MAX_PACKET_SIZE},
  {0x80U, USB_FS_MAX_PACKET_SIZE},
  {0x01U, MSC_EP_SIZE},
  {0x81U, MSC_EP_SIZE},
#endif /* USB */
};

#endif /* HAL_PCD_MODULE_ENABLED && USBCON */
