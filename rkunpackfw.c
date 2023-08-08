#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "version.h"
#include "rkflashtool.h"
#include "rkusb.h"
#include "rkboot.h"

#ifdef _WIN32       /* hack around non-posix behaviour */
#undef mkdir
#define mkdir(a,b) _mkdir(a)
int _mkdir(const char *);
#include <windows.h>
HANDLE fm;
#else
#define O_BINARY 0
#endif

void usage(void) {
    info("rkunpackfw v%d.%d\n", RKFLASHTOOL_VERSION_MAJOR,
                                 RKFLASHTOOL_VERSION_MINOR);

    fatal( "usage:\n"
          "\trkunpackfw file              \tunpack rockchip firmware\n"
         );
}

#define NEXT do { argc--;argv++; } while(0)

uint8_t *buf;
off_t size;
static unsigned int fsize, ioff, isize, noff;
static int fd;


static void write_file(const char *path, uint8_t *buffer, unsigned int length) {
    int img;
    if ((img = open(path, O_BINARY | O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1 ||
               write(img, buffer, length) == -1 ||
               close(img) == -1)
        fatal("%s: %s\n", path, strerror(errno));
}


void install_rkfw(void) {
    char *chip = NULL;
     uint8_t *p;
    int count;
        const char *name, *path, *sep;
    char dir[PATH_MAX];

    info("RKFW signature detected\n");
    info("version: %d.%d.%d\n", buf[9], buf[8], (buf[7]<<8)+buf[6]);
    info("date: %d-%02d-%02d %02d:%02d:%02d\n",
            (buf[0x0f]<<8)+buf[0x0e], buf[0x10], buf[0x11],
            buf[0x12], buf[0x13], buf[0x14]);
    switch(buf[0x15]) {
        case 0x50:  chip = "rk29xx"; break;
        case 0x60:  chip = "rk30xx"; break;
        case 0x70:  chip = "rk31xx"; break;
        case 0x80:  chip = "rk32xx"; break;
        case 0x41:  chip = "rk3368"; break;
        case 0x38:  chip = "rk3588"; break;
        default: info("You got a brand new chip (%#x), congratulations!!!\n", buf[0x15]);
    }
    info("family: %s\n", chip ? chip : "unknown");

    ioff  = GET32LE(buf+0x19);
    isize = GET32LE(buf+0x1d);

    if (memcmp(buf+ioff, "BOOT", 4)){
        // WIP: find out if this is meaningful for RK3588 
        // Signature BOOT not present in the first bytes but LDR in its place?
        info("cannot find BOOT signature... skipping\n");
    }else{
	    info("%08x-%08x %-26s (size: %u)\n", ioff, ioff + isize -1, "BOOT", isize);
	    write_file("BOOT", buf+ioff, isize);
    }

    ioff  = GET32LE(buf+0x21);
    isize = GET32LE(buf+0x25);

    if (memcmp(buf+ioff, "RKAF", 4))
        fatal("cannot find embedded RKAF update.img\n");

    buf = buf + ioff;
    size = isize;
    fsize = GET32LE(buf+4) + 4;
    if (fsize != (unsigned)size)
        info("invalid file size %u(should be %u bytes)\n", size, fsize);
    else
        info("file size matches (%u bytes)\n", fsize);

    info("manufacturer: %s\n", buf + 0x48);
    info("model: %s\n", buf + 0x08);

    count = GET32LE(buf+0x88);

    info("number of files: %d\n", count);

    for (p = &buf[0x8c]; count > 0; p += 0x70, count--) {
        name = (const char *)p;
        path = (const char *)p + 0x20;

        ioff  = GET32LE(p+0x60);
        noff  = GET32LE(p+0x64);
        isize = GET32LE(p+0x68);
        fsize = GET32LE(p+0x6c);

        if (memcmp(path, "SELF", 4) == 0) {
            info("skipping SELF entry\n");
        } else {
            info("%08x-%08x %-26s (size: %d)\n", ioff, ioff + isize - 1, path, fsize);

            // strip header and footer of parameter file
            if (memcmp(name, "parameter", 9) == 0) {
                ioff += 8;
                fsize -= 12;
            }

            sep = path;
            while ((sep = strchr(sep, '/')) != NULL) {
                memcpy(dir, path, sep - path);
                dir[sep - path] = '\0';
                if (mkdir(dir, 0755) == -1 && errno != EEXIST)
                    fatal("%s: %s\n", dir, strerror(errno));
                sep++;
            }

            write_file(path, buf+ioff, fsize);
        }
    }
}


int main(int argc, char **argv) {
    char *file;

    NEXT; if (!argc) usage();
    file = *argv;

    info ("Try to flash %s\n", file);
    if ((fd = open(file, O_BINARY | O_RDONLY)) == -1)
        fatal("%s: %s\n", file, strerror(errno));

    if ((size = lseek(fd, 0, SEEK_END)) == -1)
        fatal("%s: %s\n", file, strerror(errno));

    if ((buf = mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0))
                                                    == MAP_FAILED)
        fatal("%s: %s\n", file, strerror(errno));

    if (!memcmp(buf, "RKFW", 4))  {
        install_rkfw();
    }
    else {
        fatal("%s: invalid signature\n", file);
    }

    munmap(buf, size);

    close(fd);
    return 0;
}
