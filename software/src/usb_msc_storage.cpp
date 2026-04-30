/**
 * USB MSC virtual FAT12 RAM disk – storage callbacks + initialisation.
 *
 * Disk layout (8 sectors × 512 B = 4 KB):
 *   Sector 0 : BPB / boot record
 *   Sector 1 : FAT1  (1 FAT, 1 sector)
 *   Sector 2 : Root directory (16 entries × 32 B)
 *   Sector 3 : Cluster 2 → color.txt content  ← written by the host
 *   Sectors 4-7 : free
 */
#ifdef USBCON

#include <string.h>
#include "usb_msc_storage.h"
#include "usbd_msc.h"
#include "usbd_desc.h"
#include "usbd_if.h"

extern "C" {

/* ============================================================
 * Virtual RAM disk
 * ============================================================ */
#define DISK_SECTORS  8U
#define SECTOR_SIZE   512U

static uint8_t disk[DISK_SECTORS][SECTOR_SIZE];
static bool    disk_initialized = false;
volatile bool  disk_dirty       = false;

static void disk_init(void)
{
    memset(disk, 0, sizeof(disk));

    /* ---- Sector 0: BPB ---- */
    uint8_t *b = disk[0];
    b[0]=0xEB; b[1]=0x3C; b[2]=0x90;       /* JMP + NOP              */
    memcpy(&b[3],  "MSDOS5.0", 8);          /* OEM name               */
    b[11]=0x00; b[12]=0x02;                 /* BytsPerSec = 512       */
    b[13]=0x01;                             /* SecPerClus = 1         */
    b[14]=0x01; b[15]=0x00;                 /* RsvdSecCnt = 1         */
    b[16]=0x01;                             /* NumFATs    = 1         */
    b[17]=0x10; b[18]=0x00;                 /* RootEntCnt = 16        */
    b[19]=0x08; b[20]=0x00;                 /* TotSec16   = 8         */
    b[21]=0xF8;                             /* Media      = 0xF8      */
    b[22]=0x01; b[23]=0x00;                 /* FATSz16    = 1         */
    b[24]=0x01; b[25]=0x00;                 /* SecPerTrk  = 1         */
    b[26]=0x01; b[27]=0x00;                 /* NumHeads   = 1         */
    /* HiddSec[28-31] = 0, TotSec32[32-35] = 0                        */
    b[36]=0x00;                             /* DrvNum                 */
    b[38]=0x29;                             /* BootSig = 0x29         */
    b[39]=0x78; b[40]=0x56; b[41]=0x34; b[42]=0x12; /* VolID         */
    memcpy(&b[43], "NEOPIXEL   ", 11);      /* VolLab  (11 B)         */
    memcpy(&b[54], "FAT12   ",    8);       /* FilSysType (8 B)       */
    b[510]=0x55; b[511]=0xAA;              /* Boot signature          */

    /* ---- Sector 1: FAT1 ---- */
    /* FAT12 packing: entry0=0xFF8, entry1=0xFFF, entry2=0xFFF(EOF)   */
    uint8_t *f = disk[1];
    f[0]=0xF8; f[1]=0xFF; f[2]=0xFF;  /* entries 0 & 1              */
    f[3]=0xFF; f[4]=0x0F; f[5]=0x00;  /* entries 2 (EOF) & 3 (free) */

    /* ---- Sector 2: Root directory ---- */
    uint8_t *r = disk[2];
    /* Entry 0: volume label */
    memcpy(&r[0], "NEOPIXEL   ", 11);
    r[11] = 0x08;                      /* ATTR_VOLUME_ID             */
    /* Entry 1: color.txt */
    uint8_t *e = &r[32];
    memcpy(&e[0], "COLOR   ", 8);      /* 8.3 name                   */
    memcpy(&e[8], "TXT",      3);      /* extension                  */
    e[11] = 0x20;                      /* ATTR_ARCHIVE               */
    e[26] = 0x02; e[27] = 0x00;       /* FstClusLO = 2              */
    e[28] = 0x04;                      /* FileSize  = 4              */

    /* ---- Sector 3: color.txt initial content ---- */
    memcpy(disk[3], "red\n", 4);
}

/* ============================================================
 * SCSI Inquiry response (36 bytes)
 * ============================================================ */
static const int8_t MSC_INQUIRY_DATA[36] = {
    0x00,                                   /* Direct-access device   */
    (int8_t)0x80,                               /* Removable              */
    0x02,                                   /* SCSI-2                 */
    0x02,                                   /* Response data format   */
    0x1F,                                   /* Additional length = 31 */
    0x00, 0x00, 0x00,                       /* Reserved               */
    'S','T','M','3','2',' ',' ',' ',        /* Vendor  ID (8)         */
    'D','i','s','k',' ',' ',' ',' ',        /* Product ID (16)        */
    ' ',' ',' ',' ',' ',' ',' ',' ',
    '1','.','0','0'                         /* Version (4)            */
};

/* ============================================================
 * USBD MSC Storage callbacks
 * ============================================================ */
static int8_t STORAGE_Init(uint8_t lun)
{
    (void)lun;
    if (!disk_initialized) {
        disk_init();
        disk_initialized = true;
    }
    return USBD_OK;
}

static int8_t STORAGE_GetCapacity(uint8_t lun,
                                   uint32_t *block_num,
                                   uint16_t *block_size)
{
    (void)lun;
    *block_num  = DISK_SECTORS;
    *block_size = SECTOR_SIZE;
    return USBD_OK;
}

static int8_t STORAGE_IsReady(uint8_t lun)
{
    (void)lun;
    return USBD_OK;
}

static int8_t STORAGE_IsWriteProtected(uint8_t lun)
{
    (void)lun;
    return USBD_OK;   /* not write-protected */
}

static int8_t STORAGE_Read(uint8_t lun, uint8_t *buf,
                            uint32_t blk_addr, uint16_t blk_len)
{
    (void)lun;
    if ((blk_addr + blk_len) > DISK_SECTORS) return USBD_FAIL;
    memcpy(buf, disk[blk_addr], (size_t)blk_len * SECTOR_SIZE);
    return USBD_OK;
}

static int8_t STORAGE_Write(uint8_t lun, uint8_t *buf,
                              uint32_t blk_addr, uint16_t blk_len)
{
    (void)lun;
    if ((blk_addr + blk_len) > DISK_SECTORS) return USBD_FAIL;
    memcpy(disk[blk_addr], buf, (size_t)blk_len * SECTOR_SIZE);
    /* Signal main loop when color.txt data sector (3) is written */
    if (blk_addr <= 3U && (blk_addr + blk_len) > 3U) {
        disk_dirty = true;
    }
    return USBD_OK;
}

static int8_t STORAGE_GetMaxLun(void)
{
    return 0;   /* single LUN */
}

USBD_StorageTypeDef USBD_Storage_fops = {
    STORAGE_Init,
    STORAGE_GetCapacity,
    STORAGE_IsReady,
    STORAGE_IsWriteProtected,
    STORAGE_Read,
    STORAGE_Write,
    STORAGE_GetMaxLun,
    (int8_t *)MSC_INQUIRY_DATA,
};

/* ============================================================
 * USB MSC device initialisation
 * ============================================================ */
static USBD_HandleTypeDef hUSBD_MSC;

void MSC_init(void)
{
    disk_init();
    disk_initialized = true;

    USBD_reenumerate();          /* force host to re-enumerate        */
    USBD_Init(&hUSBD_MSC, &USBD_Desc, 0);
    USBD_RegisterClass(&hUSBD_MSC, USBD_MSC_CLASS);
    USBD_MSC_RegisterStorage(&hUSBD_MSC, &USBD_Storage_fops);
    USBD_Start(&hUSBD_MSC);
}

/* ============================================================
 * Read the color pending in color.txt (called from loop())
 * Returns a lowercase C-string, or nullptr if nothing changed.
 * ============================================================ */
const char *MSC_get_pending_color(void)
{
    if (!disk_dirty) return nullptr;
    disk_dirty = false;

    /* color.txt content is at sector 3 */
    static char buf[32];
    memset(buf, 0, sizeof(buf));

    for (int i = 0; i < 31; i++) {
        char c = (char)disk[3][i];
        if (c == '\0' || c == '\r' || c == '\n') break;
        /* lowercase */
        if (c >= 'A' && c <= 'Z') c += 32;
        buf[i] = c;
    }
    return (buf[0] != '\0') ? buf : nullptr;
}

} /* extern "C" */

#endif /* USBCON */
