#ifndef _RKBOOT_H_
#define _RKBOOT_H_

#define MAX_NAME_LEN        20

typedef enum {
	ENTRY_471       =1,
	ENTRY_472       =2,
	ENTRY_LOADER    =4,
} rk_entry_type;

#pragma pack(1)

typedef struct {
	uint16_t  year;
	uint8_t   month;
	uint8_t   day;
	uint8_t   hour;
	uint8_t   minute;
	uint8_t   second;
} rk_time;

#define BOOT_RESERVED_SIZE  57
typedef struct {
	uint32_t        tag;
	uint16_t        size;
	uint32_t        version;
	uint32_t        mergerVersion;
	rk_time        releaseTime; // TODO -- fix rktime
	uint32_t        chipType;
	uint8_t         code471Num;
	uint32_t        code471Offset;
	uint8_t         code471Size;
	uint8_t         code472Num;
	uint32_t        code472Offset;
	uint8_t         code472Size;
	uint8_t         loaderNum;
	uint32_t        loaderOffset;
	uint8_t         loaderSize;
	uint8_t         signFlag;
	uint8_t         rc4Flag;
	uint8_t         reserved[BOOT_RESERVED_SIZE];
} rk_boot_header;

typedef struct {
	uint8_t         size;
	rk_entry_type   type;
	uint16_t        name[MAX_NAME_LEN];
	uint32_t        dataOffset;
	uint32_t        dataSize;
	uint32_t        dataDelay;
} rk_boot_entry;
#pragma pack()

static inline void wide2str(const uint16_t *wide, char *str, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		str[i] = (char)(wide[i] & 0xFF);
	}
	str[len] = 0;
}

#endif
