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

#ifdef _WIN32       /* hack around non-posix behaviour */
#undef mkdir
#define mkdir(a,b) _mkdir(a)
int _mkdir(const char *);
#include <windows.h>
HANDLE fm;
#else
#define O_BINARY 0
#endif

#define NEXT do { argc--;argv++; } while(0)
/* magic and hash size */
#define LOADER_MAGIC_SIZE 8
#define LOADER_HASH_SIZE 32

typedef struct  {
	uint8_t magic[LOADER_MAGIC_SIZE]; /* magic */
	uint32_t version;
	uint32_t reserved0;
	uint32_t loader_load_addr;      /* physical load addr */
	uint32_t loader_load_size;      /* size in bytes */
	uint32_t crc32;                 /* crc32 */
	uint32_t hash_len;              /* 20 or 32 , 0 is no hash */
	uint8_t hash[LOADER_HASH_SIZE]; /* sha */

	uint8_t reserved[1024 - 32 - 32];
	uint32_t signTag;     /* 0x4E474953 */
	uint32_t signlen;     /* maybe 128 or 256 */
	uint8_t rsaHash[256]; /* maybe 128 or 256, using max size 256 */
	uint8_t reserved2[2048 - 1024 - 256 - 8];
} loader_hdr;

typedef struct {
	char magic[4]; /* tag, "RSCE" */
	uint16_t resource_ptn_version;
	uint16_t index_tbl_version;
	uint8_t header_size;    /* blocks, size of ptn header. */
	uint8_t tbl_offset;     /* blocks, offset of index table. */
	uint8_t tbl_entry_size; /* blocks, size of index table's entry. */
	uint32_t tbl_entry_num; /* numbers of index table's entry. */
} resource_header;

typedef struct {
	char tag[4]; /* tag, "ENTR" */
	char path[256];
	uint32_t content_offset; /* blocks, offset of resource content. */
	uint32_t content_size;   /* bytes, size of resource content. */
} resource_entry;

uint8_t *buf;
off_t size;
static int fd;

void usage(void) {
    info("rkunpackimg v%d.%d\n", RKFLASHTOOL_VERSION_MAJOR,
                                 RKFLASHTOOL_VERSION_MINOR);

    fatal( "usage:\n"
          "\trkunpackimg infile [outfile]        \tunpack rockchip image\n"
         );
}

static void write_file(const char *path, uint8_t *buffer, unsigned int length) {
    int img;
    if ((img = open(path, O_BINARY | O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1 ||
               write(img, buffer, length) == -1 ||
               close(img) == -1)
        fatal("%s: %s\n", path, strerror(errno));
}

void unpack_resource_image() {
	resource_header *ptr_resource_header;
	
	info("Resource signature detected\n");
	ptr_resource_header = (resource_header *) buf;
	for (int i = 0; i < (int) ptr_resource_header->tbl_entry_num; i++) {		
		resource_entry *ptr_entry = (resource_entry *) (buf +  (ptr_resource_header->tbl_offset << 9) + ( i * (ptr_resource_header -> tbl_entry_size << 9)));
		info("writing %s\n", ptr_entry->path);
		write_file(ptr_entry->path, ( buf + ( ptr_entry->content_offset << 9 ) ), ptr_entry->content_size);
	}
	
	info("Done!\n");
}

void unpack_kernel_image(char *ofile) {	
	info("KRNL signature detected\n");
	write_file(ofile, buf + 8 , size - 4);
	info("Done!\n");
}

void unpack_loader_image(char *ofile) {
	loader_hdr *ptr_loader_hdr;
	
	info("Loader signature detected\n");
	ptr_loader_hdr = (loader_hdr *) buf;
	
	info("version: %d\n", ptr_loader_hdr->version);
	info("load addr: %08x\n", ptr_loader_hdr->loader_load_addr);	
	info("hash len: %d\n", ptr_loader_hdr->hash_len);
	
	write_file(ofile, buf + sizeof(loader_hdr) , ptr_loader_hdr->loader_load_size);
	info("Done!\n");
}

int main(int argc, char **argv) {
    char *file;
    char *ofile;

    NEXT; if (!argc) usage();
    file = argv[0];
    ofile = argv[1];
    
    info ("Try to unpack %s\n", file);
    if ((fd = open(file, O_BINARY | O_RDONLY)) == -1)
        fatal("%s: %s\n", file, strerror(errno));

    if ((size = lseek(fd, 0, SEEK_END)) == -1)
        fatal("%s: %s\n", file, strerror(errno));

    if ((buf = mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0))
                                                    == MAP_FAILED)
        fatal("%s: %s\n", file, strerror(errno));

    if (!memcmp(buf, "KRNL", 4))  {
        if (argc != 2) {
		usage();
	}
	unpack_kernel_image(ofile);
    } else if (!memcmp(buf, "LOADER  ", 8)) {
	if (argc != 2) {
		usage();
	}
	unpack_loader_image(ofile);
    } else if (!memcmp(buf, "RSCE", 4)) {
	unpack_resource_image();
    }
    else {
        fatal("%s: invalid signature\n", file);
    }

    munmap(buf, size);

    close(fd);
    return 0;
}
