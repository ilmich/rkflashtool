#ifndef _RKUSB_H_
#define _RKUSB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libusb.h>
#include "rkcrc.h"

static const char *const strings[2] = { "info", "fatal" };

static void info_and_fatal(const int s, const int cr, char *f, ...) {
    va_list ap;
    va_start(ap,f);
    fprintf(stderr, "%srkflashtool: %s: ", cr ? "\r" : "", strings[s]);
    vfprintf(stderr, f, ap);
    va_end(ap);
    if (s) exit(s);
}

#define info(...)    info_and_fatal(0, 0, __VA_ARGS__)
#define infocr(...)  info_and_fatal(0, 1, __VA_ARGS__)
#define fatal(...)   info_and_fatal(1, 0, __VA_ARGS__)

#define RKFT_USB_MODE_MASKROM   0x200
#define RKFT_USB_MODE_LOADER    0x201

#define RKFT_BLOCKSIZE      0x4000      /* must be multiple of 512 */
#define RKFT_IDB_DATASIZE   0x200
#define RKFT_IDB_BLOCKSIZE  0x210
#define RKFT_IDB_INCR       0x20
#define RKFT_MEM_INCR       0x80
#define RKFT_OFF_INCR       (RKFT_BLOCKSIZE>>9)
#define MAX_PARAM_LENGTH    (128*512-12) /* cf. MAX_LOADER_PARAM in rkloader */
#define SDRAM_BASE_ADDRESS  0x60000000

#define RKFT_CMD_TESTUNITREADY      0x80000600
#define RKFT_CMD_READFLASHID        0x80000601
#define RKFT_CMD_READFLASHINFO      0x8000061a
#define RKFT_CMD_READCHIPINFO       0x8000061b
#define RKFT_CMD_READEFUSE          0x80000620

#define RKFT_CMD_SETDEVICEINFO      0x00000602
#define RKFT_CMD_ERASESYSTEMDISK    0x00000616
#define RKFT_CMD_SETRESETFLASG      0x0000061e
#define RKFT_CMD_RESETDEVICE        0x000006ff

#define RKFT_CMD_TESTBADBLOCK       0x80000a03
#define RKFT_CMD_READSECTOR         0x80000a04
#define RKFT_CMD_READLBA            0x80000a14
#define RKFT_CMD_READSDRAM          0x80000a17
#define RKFT_CMD_UNKNOWN1           0x80000a21

#define RKFT_CMD_WRITESECTOR        0x00000a05
#define RKFT_CMD_ERASESECTORS       0x00000a06
#define RKFT_CMD_UNKNOWN2           0x00000a0b
#define RKFT_CMD_WRITELBA           0x00000a15
#define RKFT_CMD_WRITESDRAM         0x00000a18
#define RKFT_CMD_EXECUTESDRAM       0x00000a19
#define RKFT_CMD_WRITEEFUSE         0x00000a1f
#define RKFT_CMD_UNKNOWN3           0x00000a22

#define RKFT_CMD_WRITESPARE         0x80001007
#define RKFT_CMD_READSPARE          0x80001008

#define RKFT_CMD_LOWERFORMAT        0x0000001c
#define RKFT_CMD_WRITENKB           0x00000030

#define SETBE16(a, v) do { \
                        ((uint8_t*)a)[1] =  v      & 0xff; \
                        ((uint8_t*)a)[0] = (v>>8 ) & 0xff; \
                      } while(0)

#define SETBE32(a, v) do { \
                        ((uint8_t*)a)[3] =  v      & 0xff; \
                        ((uint8_t*)a)[2] = (v>>8 ) & 0xff; \
                        ((uint8_t*)a)[1] = (v>>16) & 0xff; \
                        ((uint8_t*)a)[0] = (v>>24) & 0xff; \
                      } while(0)

static struct t_pid {
    uint16_t pid;
    char name[32];
} pidtab[] = {
    { 0x281a, "RK2818" },
    { 0x290a, "RK2918" },
    { 0x292a, "RK2928" },
    { 0x292c, "RK3026" },
    { 0x300a, "RK3066" },
    { 0x300b, "RK3168" },
    { 0x301a, "RK3036" },
    { 0x310a, "RK3066B" },
    { 0x310b, "RK3188" },
    { 0x310c, "RK312X" }, // Both RK3126 and RK3128
    { 0x310d, "RK3126" },
    { 0x320a, "RK3288" },
    { 0x320b, "RK322X" }, // Both RK3228 and RK3229
    { 0x320c, "RK3228H/RK3318/RK3328" },
    { 0x330a, "RK3368" },
    { 0x330c, "RK3399" },
    { 0, "" },
};

typedef struct {
    uint32_t flash_size;
    uint16_t block_size;
    uint8_t page_size;
    uint8_t ecc_bits;
    uint8_t access_time;
    uint8_t manufacturer_id;
    uint8_t chip_select;
} nand_info;

typedef struct {
    uint16_t vid;
    uint16_t pid;
    char* soc;
    uint16_t mode;
    nand_info *nand;
    libusb_context *usb_ctx;
    libusb_device_handle *usb_handle;
    uint8_t cmd[31], res[13], buf[RKFT_BLOCKSIZE];
} rkusb_device;

static const char* const manufacturer[] = {   /* NAND Manufacturers */
    "Samsung",
    "Toshiba",
    "Hynix",
    "Infineon",
    "Micron",
    "Renesas",
    "Intel",
    "UNKNOWN", /* Reserved */
    "SanDisk",
};
#define MAX_NAND_ID (sizeof manufacturer / sizeof(char *))
static int tmp;

void rkrc4(unsigned char* buf, unsigned short len)
{
    unsigned char S[256],K[256],temp;
    unsigned short i,j,t,x;
    unsigned char key[16]={124,78,3,4,85,5,9,7,45,44,123,56,23,13,23,17};

    j = 0;
    for(i=0; i<256; i++){
        S[i] = (unsigned char)i;
        j&=0x0f;
        K[i] = key[j];
        j++;
    }

    j = 0;
    for(i=0; i<256; i++){
        j = (j + S[i] + K[i]) % 256;
        temp = S[i];
        S[i] = S[j];
        S[j] = temp;
    }

    i = j = 0;
    for(x=0; x<len; x++){
        i = (i+1) % 256;
        j = (j + S[i]) % 256;
        temp = S[i];
        S[i] = S[j];
        S[j] = temp;
        t = (S[i] + (S[j] % 256)) % 256;
        buf[x] = buf[x] ^ S[t];
    }
}

void rkusb_send_reset(rkusb_device* device, uint8_t flag) {
    long int r = random();

    memset(device->cmd, 0 , 31);
    memcpy(device->cmd, "USBC", 4);

    SETBE32(device->cmd+4, r);
    SETBE32(device->cmd+12, RKFT_CMD_RESETDEVICE);
    device->cmd[16] = flag;

    libusb_bulk_transfer(device->usb_handle, 2|LIBUSB_ENDPOINT_OUT, device->cmd, sizeof(device->cmd), &tmp, 0);
}

void rkusb_send_exec(rkusb_device* device, uint32_t krnl_addr, uint32_t parm_addr) {
    long int r = random();

    memset(device->cmd, 0 , 31);
    memcpy(device->cmd, "USBC", 4);

    if (r)          SETBE32(device->cmd+4, r);
    if (krnl_addr)  SETBE32(device->cmd+17, krnl_addr);
    if (parm_addr)  SETBE32(device->cmd+22, parm_addr);
    SETBE32(device->cmd+12, RKFT_CMD_EXECUTESDRAM);

    libusb_bulk_transfer(device->usb_handle, 2|LIBUSB_ENDPOINT_OUT, device->cmd, sizeof(device->cmd), &tmp, 0);
}

void rkusb_send_cmd(rkusb_device* device, uint32_t command, uint32_t offset, uint16_t nsectors) {
    long int r = random();

    memset(device->cmd, 0 , 31);
    memcpy(device->cmd, "USBC", 4);

    if (r)          SETBE32(device->cmd+4, r);
    if (offset)     SETBE32(device->cmd+17, offset);
    if (nsectors)   SETBE16(device->cmd+22, nsectors);
    if (command)    SETBE32(device->cmd+12, command);

    libusb_bulk_transfer(device->usb_handle, 2|LIBUSB_ENDPOINT_OUT, device->cmd, sizeof(device->cmd), &tmp, 0);
}

void rkusb_recv_res(rkusb_device* device) {
    libusb_bulk_transfer(device->usb_handle, 1|LIBUSB_ENDPOINT_IN, device->res, sizeof(device->res), &tmp, 0);
}

void rkusb_send_buf(rkusb_device* device, unsigned int s) {
    libusb_bulk_transfer(device->usb_handle, 2|LIBUSB_ENDPOINT_OUT, device->buf, s, &tmp, 0);
}

void rkusb_recv_buf(rkusb_device* device, unsigned int s) {
    libusb_bulk_transfer(device->usb_handle, 1|LIBUSB_ENDPOINT_IN, device->buf, s, &tmp, 0);
}

void rkusb_disconnect(rkusb_device *device) {
    if (device) {
        info("disconnect device\r\n");
        libusb_release_interface(device->usb_handle, 0);
        libusb_close(device->usb_handle);
    }
    libusb_exit(device->usb_ctx);
    free(device);
}

rkusb_device *rkusb_allocate_device() {
    return malloc(sizeof(rkusb_device));
}

rkusb_device *rkusb_connect_device() {
    struct libusb_device_descriptor desc;
    struct t_pid *ppid = pidtab;

    rkusb_device *device = rkusb_allocate_device();

    /* Initialize libusb */
    if (libusb_init(&device->usb_ctx)) return NULL; //fatal("cannot init libusb\n");

    libusb_set_option(device->usb_ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO );

    /* Detect connected RockChip device */
    device->usb_handle = 0;

    while ( !device->usb_handle && ppid->pid ) {
        device->usb_handle = libusb_open_device_with_vid_pid(device->usb_ctx, 0x2207, ppid->pid);

        if (device->usb_handle) {
            info("Detected %s...\n", ppid->name);
            device->vid = 0x2207;
            device->pid = ppid->pid;
            device->soc = ppid->name;
            break;
        }
        ppid++;
    }

    if (!device->usb_handle) return NULL; //fatal("cannot open device\n");

    /* Connect to device */
    if (libusb_kernel_driver_active(device->usb_handle, 0) == 1) {
        info("kernel driver active\n");
        if (!libusb_detach_kernel_driver(device->usb_handle, 0))
            info("driver detached\n");
    }

    if (libusb_claim_interface(device->usb_handle, 0) < 0)
        fatal("cannot claim interface\n");
    info("interface claimed\n");

    if (libusb_get_device_descriptor(libusb_get_device(device->usb_handle), &desc) != 0)
        fatal("cannot get device descriptor\n");

    //info("bdcUSB %04x\r\n", desc.bcdUSB);
    device->mode = desc.bcdUSB;

    return device;
}

int rkusb_file_size(FILE *fp) {
    int sz = 0;

    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    return sz;
}

int rkusb_load_vendor_code(uint8_t **buffs, char *filename) {
    int size;
    FILE *fp = NULL;
    uint16_t crc16 = 0xffff;

    info("load %s\n", filename);
    fp = fopen(filename , "r");
    size = rkusb_file_size(fp);
    info("size of file %s - %d\n", filename, size);
    size = ((size % 2048) == 0) ? size :  ((size/2048) + 1) *2048;
    info("size of padded buffer %s - %d\n", filename, size);
    *buffs = malloc(size + 5); //make room for crc
    memset (*buffs, 0, size);
    fread(*buffs, size, 1 , fp);
    rkrc4(*buffs, size);
    crc16 = rkcrc16(crc16, *buffs, size);
    (*buffs)[size++] = crc16 >> 8;
    (*buffs)[size++] = crc16 & 0xff;
    info("crc calculated %04x\n", crc16);

    return size;
}

void rkusb_send_vendor_code(rkusb_device* device, uint8_t *buffs, int size, int code) {
    while (size > 4096) {
        libusb_control_transfer(device->usb_handle, LIBUSB_REQUEST_TYPE_VENDOR, 12, 0, code, buffs, 4096, 0);
            buffs += 4096;
            size -= 4096;
    }
    libusb_control_transfer(device->usb_handle, LIBUSB_REQUEST_TYPE_VENDOR, 12, 0, code, buffs, size, 0);
}

#endif
