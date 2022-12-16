/* rkflashtool - for RockChip based devices.
 *               (RK2808, RK2818, RK2918, RK2928, RK3066, RK3068, RK3126 and RK3188)
 *
 * Copyright (C) 2010-2014 by Ivo van Poorten, Fukaumi Naoki, Guenter Knauf,
 *                            Ulrich Prinz, Steve Wilson
 *
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
#include "rkcrc.h"
#include "rkflashtool.h"
#include "rkusb.h"

static void usage(void) {
    fatal("usage:\n"
          "\trkflashtool b [flag]            \treboot device\n"
          "\trkflashtool l file             \tload DDR init (MASK ROM MODE)\n"
          "\trkflashtool L file             \tload USB loader (MASK ROM MODE)\n"
          "\trkflashtool v                   \tread chip version\n"
          "\trkflashtool n                   \tread NAND flash info\n"
          "\trkflashtool i offset nsectors >outfile \tread IDBlocks\n"
          "\trkflashtool j offset nsectors <infile  \twrite IDBlocks\n"
          "\trkflashtool m offset nbytes   >outfile \tread SDRAM\n"
          "\trkflashtool M offset nbytes   <infile  \twrite SDRAM\n"
          "\trkflashtool B krnl_addr parm_addr      \texec SDRAM\n"
          "\trkflashtool r partname >outfile \tread flash partition\n"
          "\trkflashtool w partname <infile  \twrite flash partition\n"
          "\trkflashtool r offset nsectors >outfile \tread flash\n"
          "\trkflashtool w offset nsectors <infile  \twrite flash\n"
//          "\trkflashtool f                 >outfile \tread fuses\n"
//          "\trkflashtool g                 <infile  \twrite fuses\n"
          "\trkflashtool p >file             \tfetch parameters\n"
          "\trkflashtool P <file             \twrite parameters\n"
          "\trkflashtool e partname          \terase flash (fill with 0xff)\n"
          "\trkflashtool e offset nsectors   \terase flash (fill with 0xff)\n"
         );
}

#define NEXT do { argc--;argv++; } while(0)

int main(int argc, char **argv) {
    struct libusb_device_descriptor desc;
    int offset = 0, size = 0;
    uint8_t flag = 0;
    char action;
    char *partname = NULL;
    char *ifile = NULL;
    uint8_t *tmpBuf = NULL;

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
        if (argc != 1) usage();
            ifile = argv[0];
        break;
    case 'e':
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
        if (argc) usage();
        offset = 0;
        size   = 1024;
        break;
        break;
    default:
        usage();
    }

    /* Initialize libusb */

    if (connect_usb()) fatal("cannot open device\n");
    

    if (libusb_claim_interface(h, 0) < 0)
        fatal("cannot claim interface\n");
    info("interface claimed\n");

    if (libusb_get_device_descriptor(libusb_get_device(h), &desc) != 0)
        fatal("cannot get device descriptor\n");

    info("bdcUSB %d\r\n", desc.bcdUSB);
    if (desc.bcdUSB == 0x200)
        info("MASK ROM MODE\n");

    if (desc.bcdUSB == 0x201)
        info("LOADER MODE\n");

    switch(action) {
    case 'l':
        info("load DDR init\n");
        size = load_vendor_code(&tmpBuf, ifile);
        send_vendor_code(tmpBuf, size, 0x471);
        free(tmpBuf);
        goto exit;
    case 'L':
        info("load USB loader\n");
        size = load_vendor_code(&tmpBuf, ifile);
        send_vendor_code(tmpBuf, size, 0x472);
        free(tmpBuf);
        goto exit;
    }

    /* Initialize bootloader interface */
    send_cmd(RKFT_CMD_TESTUNITREADY, 0, 0);
    recv_res();
    usleep(20*1000);

    /* Parse partition name */
    if (partname) {
        info("working with partition: %s\n", partname);

        /* Read parameters */
        offset = 0;
        send_cmd(RKFT_CMD_READLBA, offset, RKFT_OFF_INCR);
        recv_buf(RKFT_BLOCKSIZE);
        recv_res();

        /* Check parameter length */
        uint32_t *p = (uint32_t*)buf+1;
        size = *p;
        if (size < 0 || size > MAX_PARAM_LENGTH)
          fatal("Bad parameter length!\n");

        /* Search for mtdparts */
        const char *param = (const char *)&buf[8];
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
            send_cmd(RKFT_CMD_READFLASHINFO, 0, 0);
            recv_buf(512);
            recv_res();

            nand_info *nand = (nand_info *) buf;
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
        send_reset(flag);
        recv_res();
        break;
    case 'r':   /* Read FLASH */
        while (size > 0) {
            infocr("reading flash memory at offset 0x%08x", offset);

            send_cmd(RKFT_CMD_READLBA, offset, RKFT_OFF_INCR);
            recv_buf(RKFT_BLOCKSIZE);
            recv_res();

            if (write(1, buf, RKFT_BLOCKSIZE) <= 0)
                fatal("Write error! Disk full?\n");

            offset += RKFT_OFF_INCR;
            size   -= RKFT_OFF_INCR;
        }
        fprintf(stderr, "... Done!\n");
        break;
    case 'w':   /* Write FLASH */
        while (size > 0) {
            infocr("writing flash memory at offset 0x%08x", offset);

            if (read(0, buf, RKFT_BLOCKSIZE) <= 0) {
                fprintf(stderr, "... Done!\n");
                info("premature end-of-file reached.\n");
                goto exit;
            }

            send_cmd(RKFT_CMD_WRITELBA, offset, RKFT_OFF_INCR);
            send_buf(RKFT_BLOCKSIZE);
            recv_res();

            offset += RKFT_OFF_INCR;
            size   -= RKFT_OFF_INCR;
        }
        fprintf(stderr, "... Done!\n");
        break;
    case 'p':   /* Retrieve parameters */
        {
            uint32_t *p = (uint32_t*)buf+1;

            info("reading parameters at offset 0x%08x\n", offset);

            send_cmd(RKFT_CMD_READLBA, offset, RKFT_OFF_INCR);
            recv_buf(RKFT_BLOCKSIZE);
            recv_res();

            /* Check size */
            size = *p;
            info("size:  0x%08x\n", size);
            if (size < 0 || size > MAX_PARAM_LENGTH)
                fatal("Bad parameter length!\n");

            /* Check CRC */
            uint32_t crc_buf = *(uint32_t *)(buf + 8 + size),
                     crc = 0;
            crc = rkcrc32(crc, buf + 8, size);
            if (crc_buf != crc)
              fatal("bad CRC! (%#x, should be %#x)\n", crc_buf, crc);

            if (write(1, &buf[8], size) <= 0)
                fatal("Write error! Disk full?\n");
        }
        break;
    case 'P':   /* Write parameters */
        {
            /* Header */
            strncpy((char *)buf, "PARM", 4);

            /* Content */
            int sizeRead;
            if ((sizeRead = read(0, buf + 8, RKFT_BLOCKSIZE - 8)) < 0) {
                info("read error: %s\n", strerror(errno));
                goto exit;
            }

            /* Length */
            *(uint32_t *)(buf + 4) = sizeRead;

            /* CRC */
            uint32_t crc = 0;
            crc = rkcrc32(crc, buf + 8, sizeRead);
            PUT32LE(buf + 8 + sizeRead, crc);

            /*
             * The parameter file is written at 8 different offsets:
             * 0x0000, 0x0400, 0x0800, 0x0C00, 0x1000, 0x1400, 0x1800, 0x1C00
             */

            for(offset = 0; offset < 0x2000; offset += 0x400) {
                infocr("writing flash memory at offset 0x%08x", offset);
                send_cmd(RKFT_CMD_WRITELBA, offset, RKFT_OFF_INCR);
                send_buf(RKFT_BLOCKSIZE);
                recv_res();
            }
        }
        fprintf(stderr, "... Done!\n");
        break;
    case 'm':   /* Read RAM */
        while (size > 0) {
            int sizeRead = size > RKFT_BLOCKSIZE ? RKFT_BLOCKSIZE : size;
            infocr("reading memory at offset 0x%08x size %x", offset, sizeRead);

            send_cmd(RKFT_CMD_READSDRAM, offset - SDRAM_BASE_ADDRESS, sizeRead);
            recv_buf(sizeRead);
            recv_res();

            if (write(1, buf, sizeRead) <= 0)
                fatal("Write error! Disk full?\n");

            offset += sizeRead;
            size -= sizeRead;
        }
        fprintf(stderr, "... Done!\n");
        break;
    case 'M':   /* Write RAM */
        while (size > 0) {
            int sizeRead;
            if ((sizeRead = read(0, buf, RKFT_BLOCKSIZE)) <= 0) {
                info("premature end-of-file reached.\n");
                goto exit;
            }
            infocr("writing memory at offset 0x%08x size %x", offset, sizeRead);

            send_cmd(RKFT_CMD_WRITESDRAM, offset - SDRAM_BASE_ADDRESS, sizeRead);
            send_buf(sizeRead);
            recv_res();

            offset += sizeRead;
            size -= sizeRead;
        }
        fprintf(stderr, "... Done!\n");
        break;
    case 'B':   /* Exec RAM */
        info("booting kernel...\n");
        send_exec(offset - SDRAM_BASE_ADDRESS, size - SDRAM_BASE_ADDRESS);
        recv_res();
        break;
    case 'i':   /* Read IDB */
        while (size > 0) {
            int sizeRead = size > RKFT_IDB_INCR ? RKFT_IDB_INCR : size;
            infocr("reading IDB flash memory at offset 0x%08x", offset);

            send_cmd(RKFT_CMD_READSECTOR, offset, sizeRead);
            recv_buf(RKFT_IDB_BLOCKSIZE * sizeRead);
            recv_res();

            if (write(1, buf, RKFT_IDB_BLOCKSIZE * sizeRead) <= 0)
                fatal("Write error! Disk full?\n");

            offset += sizeRead;
            size -= sizeRead;
        }
        fprintf(stderr, "... Done!\n");
        break;
    case 'j':   /* write IDB */
        while (size > 0) {
            infocr("writing IDB flash memory at offset 0x%08x", offset);

            memset(ibuf, RKFT_IDB_BLOCKSIZE, 0xff);
            if (read(0, ibuf, RKFT_IDB_DATASIZE) <= 0) {
                fprintf(stderr, "... Done!\n");
                info("premature end-of-file reached.\n");
                goto exit;
            }

            send_cmd(RKFT_CMD_WRITESECTOR, offset, 1);
            libusb_bulk_transfer(h, 2|LIBUSB_ENDPOINT_OUT, ibuf, RKFT_IDB_BLOCKSIZE, &tmp, 0);
            recv_res();
            offset += 1;
            size -= 1;
        }
        fprintf(stderr, "... Done!\n");
        break;
    case 'e':   /* Erase flash */
        memset(buf, 0xff, RKFT_BLOCKSIZE);
        while (size > 0) {
            infocr("erasing flash memory at offset 0x%08x", offset);

            send_cmd(RKFT_CMD_WRITELBA, offset, RKFT_OFF_INCR);
            send_buf(RKFT_BLOCKSIZE);
            recv_res();

            offset += RKFT_OFF_INCR;
            size   -= RKFT_OFF_INCR;
        }
        fprintf(stderr, "... Done!\n");
        break;
    case 'v':   /* Read Chip Version */
        send_cmd(RKFT_CMD_READCHIPINFO, 0, 0);
        recv_buf(16);
        recv_res();

        info("chip version: %c%c%c%c-%c%c%c%c.%c%c.%c%c-%c%c%c%c\n",
            buf[ 3], buf[ 2], buf[ 1], buf[ 0],
            buf[ 7], buf[ 6], buf[ 5], buf[ 4],
            buf[11], buf[10], buf[ 9], buf[ 8],
            buf[15], buf[14], buf[13], buf[12]);
        break;
    case 'n':   /* Read NAND Flash Info */
    {
        send_cmd(RKFT_CMD_READFLASHID, 0, 0);
        recv_buf(5);
        recv_res();

        info("Flash ID: %02x %02x %02x %02x %02x\n",
            buf[0], buf[1], buf[2], buf[3], buf[4]);

        send_cmd(RKFT_CMD_READFLASHINFO, 0, 0);
        recv_buf(512);
        recv_res();

        nand_info *nand = (nand_info *) buf;
        uint8_t id = nand->manufacturer_id,
                cs = nand->chip_select;

        info("Flash Info:\n"
             "\tManufacturer: %s (%d)\n"
             "\tFlash Size: %dMB\n"
             "\tBlock Size: %dKB\n"
             "\tPage Size: %dKB\n"
             "\tECC Bits: %d\n"
             "\tAccess Time: %d\n"
             "\tFlash CS:%s%s%s%s\n",

             /* Manufacturer */
             id < MAX_NAND_ID ? manufacturer[id] : "Unknown",
             id,

             nand->flash_size >> 11, /* Flash Size */
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
    libusb_release_interface(h, 0);
    libusb_close(h);
    libusb_exit(c);
    return 0;
}
