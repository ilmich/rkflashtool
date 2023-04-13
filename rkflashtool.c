/* rkflashtool - for RockChip based devices.
 *               (RK2808, RK2818, RK2918, RK2928, RK3066, RK3068, RK3126 and RK3188)
 *
 * Copyright (C) 2010-2014 by Ivo van Poorten, Fukaumi Naoki, Guenter Knauf,
 *                            Ulrich Prinz, Steve Wilson
 * Copyright (C) 2022-2023 by ilmich < ardutu at gmail dot con > 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* hack to set binary mode for stdin / stdout on Windows */
#ifdef _WIN32
#include <fcntl.h>
int _CRT_fmode = _O_BINARY;
#endif

#include "version.h"
#include "rkbin.h"
#include "rkcrc.h"
#include "rkflashtool.h"
#include "rkusb.h"

static void usage(void) {
    fatal("usage:\n"
          "\trkflashtool b [flag]            \treboot device\n"
          "\trkflashtool d > outfile         \tfump full internal memory to image file\n"
          "\trkflashtool e                   \twipe flash\n"
          "\trkflashtool e offset nsectors   \terase flash (fill with 0xff)\n"
          "\trkflashtool f file              \tflash image file\n"
          "\trkflashtool l file              \tload DDR init (MASK ROM MODE)\n"
          "\trkflashtool L file              \tload USBPLUG loader (MASK ROM MODE)\n"
          "\trkflashtool n                   \tread nand flash info\n"
          "\trkflashtool u                   \ttry to discover device and init rockusb protocol (MASK ROM MODE)\n"
          "\trkflashtool v                   \tread chip version\n"
          "\trkflashtool z                   \tlist partitions\n"
//        "\trkflashtool i offset nsectors >outfile \tread IDBlocks\n"
//        "\trkflashtool j offset nsectors <infile  \twrite IDBlocks\n"
//        "\trkflashtool m offset nbytes   >outfile \tread SDRAM\n"
//        "\trkflashtool M offset nbytes   <infile  \twrite SDRAM\n"
//        "\trkflashtool B krnl_addr parm_addr      \texec SDRAM\n"
//        "\trkflashtool r partname >outfile \tread flash partition\n"
//        "\trkflashtool w partname <infile  \twrite flash partition\n"
//        "\trkflashtool r offset nsectors >outfile \tread flash\n"
//        "\trkflashtool w offset nsectors <infile  \twrite flash\n"
//        "\trkflashtool f                 >outfile \tread fuses\n"
//        "\trkflashtool g                 <infile  \twrite fuses\n"
//        "\trkflashtool p >file             \tfetch parameters\n"
//        "\trkflashtool P <file             \twrite parameters\n"
//        "\trkflashtool e partname          \terase flash (fill with 0xff)\n"
         );
}

#define NEXT do { argc--;argv++; } while(0)

int main(int argc, char **argv) {
    FILE *fp = NULL;
    static uint8_t ibuf[RKFT_IDB_BLOCKSIZE];
    int offset = 0, size = 0, wipe = 0;
    uint8_t flag = 0, *tmpBuf = NULL;
    char action, *partname = NULL, *ifile = NULL;    
    rkusb_device *di = NULL;
    nand_info *nand = NULL;
    rkbin_entry *rkbin = NULL;

    info("rkflashtool v%d.%d\n", RKFLASHTOOL_VERSION_MAJOR,
                                 RKFLASHTOOL_VERSION_MINOR);

    NEXT; if (!argc) usage();

    action = **argv; NEXT;

    switch(action) {
    case 'b':
        if (argc > 1) usage();
        else if (argc == 1)
            flag = strtoul(argv[0], NULL, 0);
        break;
    case 'l':
    case 'L':
    case 'f':
        if (argc != 1) usage();
            ifile = argv[0];
        break;
    case 'e':
        if (argc > 2) usage();    
        if (argc == 1) {
            partname = argv[0];
        } else if (argc == 2) {
            offset = strtoul(argv[0], NULL, 0);
            size   = strtoul(argv[1], NULL, 0);
        } else {
            wipe = 1;
        }
        break;
    case 'r':
    case 'w':
        if (argc < 1 || argc > 2) usage();    
        if (argc == 1) {
            partname = argv[0];
        } else {
            offset = strtoul(argv[0], NULL, 0);
            size   = strtoul(argv[1], NULL, 0);
        }
        break;
    case 'm':
    case 'M':
    case 'B':
    case 'i':
    case 'j':
        if (argc != 2) usage();
        offset = strtoul(argv[0], NULL, 0);
        size   = strtoul(argv[1], NULL, 0);
        break;
    case 'n':
    case 'v':
    case 'p':
    case 'P':
    case 'd':
    case 'u':
        if (argc) usage();
        offset = 0;
        size   = 1024;
        break;
        break;
    default:
        usage();
    }

    /* Initialize libusb */
    if ( !(di = rkusb_connect_device()) ) fatal("cannot open device\n");

    if (di->mode == RKFT_USB_MODE_MASKROM)
        info("MASK ROM MODE\n");

    if (di->mode == RKFT_USB_MODE_LOADER)
        info("LOADER MODE\n");

    switch(action) {
    case 'u':
        info("not yet implemented");
        goto exit;
    case 'l':
        info("load DDR init\n");
        size = rkusb_load_vendor_code(&tmpBuf, ifile);
        rkusb_send_vendor_code(di, tmpBuf, size, 0x471);
        free(tmpBuf);
        goto exit;
    case 'L':
        info("load USB loader\n");
        size = rkusb_load_vendor_code(&tmpBuf, ifile);
        rkusb_send_vendor_code(di, tmpBuf, size, 0x472);
        free(tmpBuf);
        goto exit;
    }

    /* Initialize bootloader interface */
    rkusb_send_cmd(di, RKFT_CMD_TESTUNITREADY, 0, 0);
    rkusb_recv_res(di);
    usleep(20*1000);

    /* Parse partition name */
    if (partname) {
        info("working with partition: %s\n", partname);

        /* Read parameters */
        offset = 0;
        rkusb_send_cmd(di, RKFT_CMD_READLBA, offset, RKFT_OFF_INCR);
        rkusb_recv_buf(di, RKFT_BLOCKSIZE);
        rkusb_recv_res(di);

        /* Check parameter length */
        uint32_t *p = (uint32_t*)di->buf+1;
        size = *p;
        if (size < 0 || size > MAX_PARAM_LENGTH)
          fatal("Bad parameter length!\n");

        /* Search for mtdparts */
        const char *param = (const char *)&di->buf[8];
        const char *mtdparts = strstr(param, "mtdparts=");
        if (!mtdparts) {
            info("Error: 'mtdparts' not found in command line.\n");
            goto exit;
        }

        /* Search for '(partition_name)' */
        char partexp[256];
        snprintf(partexp, 256, "(%s)", partname);
        char *par = strstr(mtdparts, partexp);
        if (!par) {
            info("Error: Partition '%s' not found.\n", partname);
            goto exit;
        }

        /* Cut string by NULL-ing just before (partition_name) */
        par[0] = '\0';

        /* Search for '@' sign */
        char *arob = strrchr(mtdparts, '@');
        if (!arob) {
            info("Error: Bad syntax in mtdparts.\n");
            goto exit;
        }

        offset = strtoul(arob+1, NULL, 0);
        info("found offset: %#010x\n", offset);

        /* Cut string by NULL-ing just before '@' sign */
        arob[0] = '\0';

        /* Search for '-' sign (if last partition) */
        char *minus = strrchr(mtdparts, '-');
        if (minus) {

            /* Read size from NAND info */
            rkusb_send_cmd(di, RKFT_CMD_READFLASHINFO, 0, 0);
            rkusb_recv_buf(di, 512);
            rkusb_recv_res(di);

            nand_info *nand = (nand_info *) di->buf;
            size = nand->flash_size - offset;

            info("partition extends up to the end of NAND (size: 0x%08x).\n", size);
            goto action;
        }

        /* Search for ',' sign */
        char *comma = strrchr(mtdparts, ',');
        if (comma) {
            size = strtoul(comma+1, NULL, 0);
            info("found size: %#010x\n", size);
            goto action;
        }

        /* Search for ':' sign (if first partition) */
        char *colon = strrchr(mtdparts, ':');
        if (colon) {
            size = strtoul(colon+1, NULL, 0);
            info("found size: %#010x\n", size);
            goto action;
        }

        /* Error: size not found! */
        info("Error: Bad syntax for partition size.\n");
        goto exit;
    }

action:
    /* Check and execute command */

    switch(action) {
    case 'b':   /* Reboot device */
        info("rebooting device...\n");
        rkusb_send_reset(di, flag);
        rkusb_recv_res(di);
        break;
    case 'd':   /* Read FLASH */
        rkusb_send_cmd(di, RKFT_CMD_READFLASHINFO, 0, 0);
        rkusb_recv_buf(di, 512);
        rkusb_recv_res(di);

        nand = (nand_info *) di->buf;
        size = nand->flash_size; /* Flash size in sectors*/
        while (size > 0) {
            infocr("reading flash memory at offset 0x%08x", offset);

            rkusb_send_cmd(di, RKFT_CMD_READLBA, offset, RKFT_OFF_INCR);
            rkusb_recv_buf(di, RKFT_BLOCKSIZE);
            rkusb_recv_res(di);

            if (write(1, di->buf, RKFT_BLOCKSIZE) <= 0)
                fatal("Write error! Disk full?\n");

            offset += RKFT_OFF_INCR;
            size   -= RKFT_OFF_INCR;
        }
        info("... Done!\n");
        break;
    case 'r':   /* Read FLASH */
        while (size > 0) {
            infocr("reading flash memory at offset 0x%08x", offset);

            rkusb_send_cmd(di, RKFT_CMD_READLBA, offset, RKFT_OFF_INCR);
            rkusb_recv_buf(di, RKFT_BLOCKSIZE);
            rkusb_recv_res(di);

            if (write(1, di->buf, RKFT_BLOCKSIZE) <= 0)
                fatal("Write error! Disk full?\n");

            offset += RKFT_OFF_INCR;
            size   -= RKFT_OFF_INCR;
        }
        info("... Done!\n");
        break;
    case 'f':   /* Write FLASH */
        rkusb_send_cmd(di, RKFT_CMD_READFLASHINFO, 0, 0);
        rkusb_recv_buf(di, 512);
        rkusb_recv_res(di);

        nand = (nand_info *) di->buf;

        fp = fopen(ifile , "r");
        size = rkusb_file_size(fp) >> 9;
        if ( size > nand->flash_size ) {
            fatal("File too big!!\n");
        }
        while (size > 0) {
            infocr("writing flash memory at offset 0x%08x", offset);

            if (fread(di->buf, RKFT_BLOCKSIZE, 1 , fp) <= 0) {
                info("... Done!\n");
                info("premature end-of-file reached.\n");
                goto exit;
            }

            rkusb_send_cmd(di, RKFT_CMD_WRITELBA, offset, RKFT_OFF_INCR);
            rkusb_send_buf(di, RKFT_BLOCKSIZE);
            rkusb_recv_res(di);

            offset += RKFT_OFF_INCR;
            size   -= RKFT_OFF_INCR;
        }
        info("... Done!\n");
        fclose(fp);
        break;
    case 'w':   /* Write FLASH */
        while (size > 0) {
            infocr("writing flash memory at offset 0x%08x", offset);

            if (read(0, di->buf, RKFT_BLOCKSIZE) <= 0) {
                info("... Done!\n");
                info("premature end-of-file reached.\n");
                goto exit;
            }

            rkusb_send_cmd(di, RKFT_CMD_WRITELBA, offset, RKFT_OFF_INCR);
            rkusb_send_buf(di, RKFT_BLOCKSIZE);
            rkusb_recv_res(di);

            offset += RKFT_OFF_INCR;
            size   -= RKFT_OFF_INCR;
        }
        info("... Done!\n");
        break;
    case 'p':   /* Retrieve parameters */
        {
            uint32_t *p = (uint32_t*)di->buf+1;

            info("reading parameters at offset 0x%08x\n", offset);

            rkusb_send_cmd(di, RKFT_CMD_READLBA, offset, RKFT_OFF_INCR);
            rkusb_recv_buf(di, RKFT_BLOCKSIZE);
            rkusb_recv_res(di);

            /* Check size */
            size = *p;
            info("size:  0x%08x\n", size);
            if (size < 0 || size > MAX_PARAM_LENGTH)
                fatal("Bad parameter length!\n");

            /* Check CRC */
            uint32_t crc_buf = *(uint32_t *)(di->buf + 8 + size),
                     crc = 0;
            crc = rkcrc32(crc, di->buf + 8, size);
            if (crc_buf != crc)
              fatal("bad CRC! (%#x, should be %#x)\n", crc_buf, crc);

            if (write(1, &di->buf[8], size) <= 0)
                fatal("Write error! Disk full?\n");
        }
        break;
    case 'P':   /* Write parameters */
        {
            /* Header */
            memcpy((char *)di->buf, "PARM", 4);

            /* Content */
            int sizeRead;
            if ((sizeRead = read(0, di->buf + 8, RKFT_BLOCKSIZE - 8)) < 0) {
                info("read error: %s\n", strerror(errno));
                goto exit;
            }

            /* Length */
            *(uint32_t *)(di->buf + 4) = sizeRead;

            /* CRC */
            uint32_t crc = 0;
            crc = rkcrc32(crc, di->buf + 8, sizeRead);
            PUT32LE(di->buf + 8 + sizeRead, crc);

            /*
             * The parameter file is written at 8 different offsets:
             * 0x0000, 0x0400, 0x0800, 0x0C00, 0x1000, 0x1400, 0x1800, 0x1C00
             */

            for(offset = 0; offset < 0x2000; offset += 0x400) {
                infocr("writing flash memory at offset 0x%08x", offset);
                rkusb_send_cmd(di, RKFT_CMD_WRITELBA, offset, RKFT_OFF_INCR);
                rkusb_send_buf(di, RKFT_BLOCKSIZE);
                rkusb_recv_res(di);
            }
        }
        info("... Done!\n");
        break;
    case 'm':   /* Read RAM */
        while (size > 0) {
            int sizeRead = size > RKFT_BLOCKSIZE ? RKFT_BLOCKSIZE : size;
            infocr("reading memory at offset 0x%08x size %x", offset, sizeRead);

            rkusb_send_cmd(di, RKFT_CMD_READSDRAM, offset - SDRAM_BASE_ADDRESS, sizeRead);
            rkusb_recv_buf(di, sizeRead);
            rkusb_recv_res(di);

            if (write(1, di->buf, sizeRead) <= 0)
                fatal("Write error! Disk full?\n");

            offset += sizeRead;
            size -= sizeRead;
        }
        info("... Done!\n");
        break;
    case 'M':   /* Write RAM */
        while (size > 0) {
            int sizeRead;
            if ((sizeRead = read(0, di->buf, RKFT_BLOCKSIZE)) <= 0) {
                info("premature end-of-file reached.\n");
                goto exit;
            }
            infocr("writing memory at offset 0x%08x size %x", offset, sizeRead);

            rkusb_send_cmd(di,RKFT_CMD_WRITESDRAM, offset - SDRAM_BASE_ADDRESS, sizeRead);
            rkusb_send_buf(di,sizeRead);
            rkusb_recv_res(di);

            offset += sizeRead;
            size -= sizeRead;
        }
        info("... Done!\n");
        break;
    case 'B':   /* Exec RAM */
        info("booting kernel...\n");
        rkusb_send_exec(di, offset - SDRAM_BASE_ADDRESS, size - SDRAM_BASE_ADDRESS);
        rkusb_recv_res(di);
        break;
    case 'i':   /* Read IDB */
        while (size > 0) {
            int sizeRead = size > RKFT_IDB_INCR ? RKFT_IDB_INCR : size;
            infocr("reading IDB flash memory at offset 0x%08x", offset);

            rkusb_send_cmd(di, RKFT_CMD_READSECTOR, offset, sizeRead);
            rkusb_recv_buf(di,RKFT_IDB_BLOCKSIZE * sizeRead);
            rkusb_recv_res(di);

            if (write(1, di->buf, RKFT_IDB_BLOCKSIZE * sizeRead) <= 0)
                fatal("Write error! Disk full?\n");

            offset += sizeRead;
            size -= sizeRead;
        }
        info("... Done!\n");
        break;
    case 'j':   /* write IDB */
        while (size > 0) {
            infocr("writing IDB flash memory at offset 0x%08x", offset);

            memset(ibuf, RKFT_IDB_BLOCKSIZE, 0xff);
            if (read(0, ibuf, RKFT_IDB_DATASIZE) <= 0) {
                info("... Done!\n");
                info("premature end-of-file reached.\n");
                goto exit;
            }

            rkusb_send_cmd(di, RKFT_CMD_WRITESECTOR, offset, 1);
            libusb_bulk_transfer(di->usb_handle, 2|LIBUSB_ENDPOINT_OUT, ibuf, RKFT_IDB_BLOCKSIZE, &tmp, 0);
            rkusb_recv_res(di);
            offset += 1;
            size -= 1;
        }
        info("... Done!\n");
        break;
    case 'e':   /* Erase flash */
        if ( !wipe) {
            memset(di->buf, 0xff, RKFT_BLOCKSIZE);
            while (size > 0) {
                infocr("erasing flash memory at offset 0x%08x", offset);

                rkusb_send_cmd(di, RKFT_CMD_ERASESECTORS, offset, RKFT_OFF_INCR);
                //rkusb_send_buf(di, RKFT_BLOCKSIZE);
                rkusb_recv_res(di);

                offset += RKFT_OFF_INCR;
                size   -= RKFT_OFF_INCR;
            }
        } else {
            info("full wipe... Done!\n");
        }
        info("... Done!\n");
        break;
    case 'v':   /* Read Chip Version */

        rkusb_send_cmd(di, RKFT_CMD_READCHIPINFO, 0, 0);
        rkusb_recv_buf(di,16);
        rkusb_recv_res(di);

        info("chip version: %c%c%c%c-%c%c%c%c.%c%c.%c%c-%c%c%c%c\n",
            di->buf[ 3], di->buf[ 2], di->buf[ 1], di->buf[ 0],
            di->buf[ 7], di->buf[ 6], di->buf[ 5], di->buf[ 4],
            di->buf[11], di->buf[10], di->buf[ 9], di->buf[ 8],
            di->buf[15], di->buf[14], di->buf[13], di->buf[12]);
        break;
    case 'n':   /* Read NAND Flash Info */
    {
        rkusb_send_cmd(di, RKFT_CMD_READFLASHID, 0, 0);
        rkusb_recv_buf(di, 5);
        rkusb_recv_res(di);

        info("Flash ID: %02x %02x %02x %02x %02x\n",
            di->buf[0], di->buf[1], di->buf[2], di->buf[3], di->buf[4]);

        rkusb_send_cmd(di, RKFT_CMD_READFLASHINFO, 0, 0);
        rkusb_recv_buf(di, 512);
        rkusb_recv_res(di);

        nand_info *nand = (nand_info *) di->buf;
        uint8_t id = nand->manufacturer_id,
                cs = nand->chip_select;

        info("Flash Info:\n"
             "\tManufacturer: %s (%d)\n"
             "\tFlash Size: %dMB\n"
             "\tFlash Size: %d sectors\n"
             "\tFlash Size: %d blocks\n"
             "\tBlock Size: %dKB\n"
             "\tPage Size: %dKB\n"
             "\tECC Bits: %d\n"
             "\tAccess Time: %d\n"
             "\tFlash CS:%s%s%s%s\n",

             /* Manufacturer */
             id < MAX_NAND_ID ? manufacturer[id] : "Unknown",
             id,

             nand->flash_size >> 11, /* Flash Size */
             nand->flash_size, /* Flash Size */
             nand->flash_size / nand->block_size, /* Flash Size */
             nand->block_size >> 1,  /* Block Size */
             nand->page_size  >> 1,  /* Page Size */
             nand->ecc_bits,         /* ECC Bits */
             nand->access_time,      /* Access Time */

             /* Flash CS */
             cs & 1 ? " <0>" : "",
             cs & 2 ? " <1>" : "",
             cs & 4 ? " <2>" : "",
             cs & 8 ? " <3>" : "");
    }
    default:
        break;
    }

exit:
    /* Disconnect and close all interfaces */
    info("disconnect\r\n");
    rkusb_disconnect(di);
    return 0;
}
