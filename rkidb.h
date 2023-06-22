#ifndef _RKIDB_H_
#define _RKIDB_H_

#pragma pack(1)
typedef	struct {
	uint32_t    dw_tag;
	uint8_t     reserved[4];
	uint32_t    ui_rc4_flag;
	uint16_t    us_bootcode_1offset;
	uint16_t    us_bootcode_2offset;
	uint8_t	    reserved1[490];
	uint16_t    us_bootdata_size;
	uint16_t    us_bootcode_size;
	uint16_t    us_crc;
} rkidb_sector0;

typedef struct {
	uint16_t    us_sysreserved_blocks;
	uint16_t    us_disk0_size;
	uint16_t    us_disk1_size;
	uint16_t    us_disk2_size;
	uint16_t    us_disk3_size;
	uint32_t    ui_chip_tag;
	uint32_t    ui_machine_id;
	uint16_t    us_loader_year;
	uint16_t    us_loader_date;
	uint16_t    us_loader_ver;
	uint16_t    us_lastloader_ver;
	uint16_t    us_readwrite_times;
	uint32_t    dw_fwver;
	uint16_t    us_machine_info_len;
	uint8_t	    uc_machine_info[30];
	uint16_t    us_manufactory_infolen;
	uint8_t	    uc_manufactory_info[30];
	uint16_t    us_flashinfo_offset;
	uint16_t    us_flashinfo_len;
	uint8_t	    reserved[384];
	uint32_t    ui_flashsize;
	uint8_t     reserved1;
	uint8_t     b_access_time;
	uint16_t    us_block_size;
	uint8_t     b_page_size;
	uint8_t     b_ecc_bits;
	uint8_t     reserved2[8];
	uint16_t    us_idBlock0;
	uint16_t    us_idBlock1;
	uint16_t    us_idBlock2;
	uint16_t    us_idBlock3;
	uint16_t    us_idBlock4;
} rkidb_sector1;

typedef struct {
	uint16_t    us_infosize;
	uint8_t     bChipInfo[16];
	uint8_t     reserved[473];
	uint8_t     szVcTag[3];
	uint16_t    usSec0Crc;
	uint16_t    usSec1Crc;
	uint32_t    uiBootCodeCrc;
	uint16_t    usSec3CustomDataOffset;
	uint16_t    usSec3CustomDataSize;
	uint8_t     szCrcTag[4];
	uint16_t    usSec3Crc;
} rkidb_sector2;

typedef struct {
    rkidb_sector0 sector0;
    rkidb_sector1 sector1;
    rkidb_sector2 sector2;
} rkidb;

typedef struct { // 88 bytes
	uint32_t size_and_off; // 4
	uint32_t address;      // 4
	uint32_t flag;         // 4
	uint32_t counter;      // 4
	uint8_t reserved[8];   // 8
	uint8_t hash[64];      // 64
} rkidbv2_image_entry;

/*
struct header0_info_v2 {
	uint32_t magic;
	uint8_t reserved[4];
	uint32_t size_and_nimage;
	uint32_t boot_flag;
	uint8_t reserved1[104];
	struct image_entry images[4];
	uint8_t reserved2[1064];
	uint8_t hash[512];
};
*/


typedef	struct { // 
	uint32_t    dw_tag;			// 4
	uint8_t     reserved[4];		// 4
	uint32_t    size_and_nimage;		// 4
	uint32_t    boot_flag;			// 4
	uint8_t	    reserved1[104];		// 104
	rkidbv2_image_entry images[4];     // 88 * 4 = 352
	uint8_t reserved2[40];		// 1064
} rkidbv2_sector0;

typedef struct {
	uint8_t reserved[512];
} rkidbv2_sector1;

typedef struct {
	uint8_t reserved[512];
} rkidbv2_sector2;

typedef struct {
	uint8_t    hash[512];
} rkidbv2_sector3;

typedef struct {
    rkidbv2_sector0 sector0;
    rkidbv2_sector1 sector1;
    rkidbv2_sector2 sector2;
    rkidbv2_sector3 sector3;
} rkidbv2;

#pragma pack()

#endif
