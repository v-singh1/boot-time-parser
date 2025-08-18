#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h> /* for size_t */
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#define BOOTSTAGE_PRESERVED_ADDR	0xA0000000
#define BOOTSTAGE_SIZE			0x90000
#define MCU_BOOTSTAGE_START_OFFSET	0x80000
#define MCU_BOOTRECORD_OFFSET		0x10
#define RECORD_COUNT 			256

enum {
	BOOTSTAGE_VERSION = 0,
	BOOTSTAGE_MAGIC = 0xb00757a3,
	BOOTSTAGE_DIGITS = 9,
};

struct uboot_bootstage_hdr {
	uint32_t version;//  Should equal BOOTSTAGE_VERSION /
	uint32_t count; // Number of records stored /
	uint32_t size; // Total data size (non-zero if valid) /
	uint32_t magic; // Must equal BOOTSTAGE_MAGIC /
	uint32_t next_id; // Next bootstage id to be used /
} __attribute__((packed));

struct uboot_bootstage_record {
	uint64_t time_us; // Originally 'ulong time_us' in U-Boot (32-bit) /
	uint64_t start_us; // Start time in microseconds /
	const char *name; // 32-bit pointer to the stage name /
	int flags; // Bootstage flags /
	int id; // Bootstage id
} __attribute__((packed));

typedef struct
{
    /* Name of the record profile */
    char name[24];
    /* Time measurement for this profile */
    uint64_t time;
} mcu_boot_record_profile_t;

/**
 * Boot stage record structure
 */
typedef struct
{
    /* Unique identifier for this record */
    uint32_t record_id;
    /* Count of profile records in this boot stage */
    uint32_t record_count;
    /* Start time of this boot stage */
    uint64_t start_time;
    /* Array of profile records */
    mcu_boot_record_profile_t profiles[0];
} mcu_boot_stage_record_t;

typedef struct {
	uint64_t start_time;
	uint64_t delta_time;
	char name[64];
} boot_record_t;

typedef struct {
	uint64_t ustart_time;
	uint64_t mcu_start_time;
	uint64_t uend_time;
	uint64_t kstart_time;
	uint64_t kend_time;
	int count;
	int mcu_reccount;
} boot_summary_t;

boot_record_t boot_records[RECORD_COUNT];
boot_record_t mcu_boot_records[RECORD_COUNT];
boot_summary_t boot_summary;
uint64_t prev_time = 0;

const char* bootstage_id_names[] = {
    [0] = "START",
    [1] = "CHECK_MAGIC",
    [2] = "BOOTSTAGE_ID_CHECK_HEADER",
    [3] = "BOOTSTAGE_ID_CHECK_CHECKSUM",
    [4] = "BOOTSTAGE_ID_CHECK_ARCH",
    [5] = "BOOTSTAGE_ID_CHECK_IMAGETYPE",
    [6] = "BOOTSTAGE_ID_DECOMP_IMAGE",
    [7] = "BOOTSTAGE_ID_DECOMP_UNIMPL",
    [8] = "BOOTSTAGE_ID_CHECK_BOOT_OS",
    [9] = "BOOTSTAGE_ID_CHECK_RAMDISK",
    [10] = "BOOTSTAGE_ID_RD_MAGIC",
    [11] = "BOOTSTAGE_ID_RD_HDR_CHECKSUM",
    [12] = "BOOTSTAGE_ID_COPY_RAMDISK",
    [13] = "BOOTSTAGE_ID_RAMDISK",
    [14] = "BOOTSTAGE_ID_NO_RAMDISK",
    [15] = "BOOTSTAGE_RUN_OS",
    [30] = "BOOTSTAGE_ID_NEED_RESET",
    [31] = "BOOTSTAGE_ID_POST_FAIL",
    [32] = "BOOTSTAGE_ID_POST_FAIL_R",
    [33] = "INIT_R",
    [34] = "BOOTSTAGE_ID_BOARD_GLOBAL_DATA",
    [35] = "BOOTSTAGE_ID_BOARD_INIT_SEQ",
    [36] = "BOOTSTAGE_ID_BOARD_FLASH",
    [37] = "BOOTSTAGE_ID_BOARD_FLASH_37",
    [38] = "BOOTSTAGE_ID_BOARD_ENV",
    [39] = "BOOTSTAGE_ID_BOARD_PCI",
    [40] = "BOOTSTAGE_ID_BOARD_INTERRUPTS",
    [41] = "BOOTSTAGE_ID_IDE_START",
    [42] = "BOOTSTAGE_ID_IDE_ADDR",
    [43] = "BOOTSTAGE_ID_IDE_BOOT_DEVICE",
    [44] = "BOOTSTAGE_ID_IDE_TYPE",
    [45] = "BOOTSTAGE_ID_IDE_PART",
    [46] = "BOOTSTAGE_ID_IDE_PART_INFO",
    [47] = "BOOTSTAGE_ID_IDE_PART_TYPE",
    [48] = "BOOTSTAGE_ID_IDE_PART_READ",
    [49] = "BOOTSTAGE_ID_IDE_FORMAT",
    [50] = "BOOTSTAGE_ID_IDE_CHECKSUM",
    [51] = "BOOTSTAGE_ID_IDE_READ",
    [52] = "BOOTSTAGE_ID_NAND_PART",
    [53] = "BOOTSTAGE_ID_NAND_SUFFIX",
    [54] = "BOOTSTAGE_ID_NAND_BOOT_DEVICE",
    [55] = "BOOTSTAGE_ID_NAND_AVAILABLE",
    [57] = "BOOTSTAGE_ID_NAND_TYPE",
    [58] = "BOOTSTAGE_ID_NAND_READ",
    [60] = "BOOTSTAGE_ID_NET_CHECKSUM",
    [64] = "BOOTSTAGE_NET_ETH_START",
    [65] = "BOOTSTAGE_NET_ETH_INIT",
    [80] = "BOOTSTAGE_ID_NET_START",
    [81] = "BOOTSTAGE_ID_NET_NETLOOP_OK",
    [82] = "BOOTSTAGE_ID_NET_LOADED",
    [83] = "BOOTSTAGE_ID_NET_DONE_ERR",
    [84] = "BOOTSTAGE_ID_NET_DONE",
    [90] = "BOOTSTAGE_ID_FIT_FDT_START",
    [100] = "BOOTSTAGE_ID_FIT_KERNEL_START",
    [110] = "BOOTSTAGE_ID_FIT_CONFIG",
    [111] = "BOOTSTAGE_ID_FIT_TYPE",
    [112] = "BOOTSTAGE_ID_FIT_COMPRESSION",
    [113] = "BOOTSTAGE_ID_FIT_OS",
    [114] = "BOOTSTAGE_ID_FIT_LOADADDR",
    [115] = "BOOTSTAGE_ID_OVERWRITTEN",
    [120] = "BOOTSTAGE_ID_FIT_RD_START",
    [130] = "BOOTSTAGE_ID_FIT_SETUP_START",
    [140] = "BOOTSTAGE_ID_IDE_FIT_READ",
    [141] = "BOOTSTAGE_ID_IDE_FIT_READ_OK",
    [150] = "BOOTSTAGE_ID_NAND_FIT_READ",
    [151] = "BOOTSTAGE_ID_NAND_FIT_READ_OK",
    [160] = "BOOTSTAGE_ID_FIT_LOADABLE_START",
    [170] = "BOOTSTAGE_ID_FIT_SPL_START",
    [171] = "BOOTSTAGE_AWAKE",
    [172] = "BOOTSTAGE_ID_START_TPL",
    [173] = "BOOTSTAGE_ID_END_TPL",
    [174] = "BOOTSTAGE_ID_START_SPL",
    [175] = "BOOTSTAGE_ID_END_SPL",
    [176] = "BOOTSTAGE_START_MCU",
    [177] = "BOOTSTAGE_ID_END_VPL",
    [178] = "BOOTSTAGE_START_UBOOT_F",
    [179] = "BOOTSTAGE_START_UBOOT_R",
    [180] = "BOOTSTAGE_USB_START",
    [181] = "BOOTSTAGE_ETH_START",
    [182] = "BOOTSTAGE_ID_BOOTP_START",
    [183] = "BOOTSTAGE_ID_BOOTP_STOP",
    [184] = "BOOTSTAGE_BOOTM_START",
    [185] = "BOOTSTAGE_BOOTM_HANDOFF",
    [186] = "BOOTSTAGE_MAIN_LOOP",
    [187] = "BOOTSTAGE_ENTER_CLI_LOOP",
    [188] = "BOOTSTAGE_KERNELREAD_START",
    [189] = "BOOTSTAGE_KERNELREAD_STOP",
    [190] = "BOOTSTAGE_ID_BOARD_INIT",
    [191] = "BOOTSTAGE_ID_BOARD_INIT_DONE",
    [192] = "BOOTSTAGE_ID_CPU_AWAKE",
    [193] = "BOOTSTAGE_ID_MAIN_CPU_AWAKE",
    [194] = "BOOTSTAGE_ID_MAIN_CPU_READY",
    [195] = "BOOTSTAGE_ID_ACCUM_LCD",
    [196] = "BOOTSTAGE_ID_ACCUM_SCSI",
    [197] = "BOOTSTAGE_ID_ACCUM_SPI",
    [198] = "BOOTSTAGE_ID_ACCUM_DECOMP",
    [199] = "BOOTSTAGE_ID_ACCUM_OF_LIVE",
    [200] = "BOOTSTAGE_ID_FPGA_INIT",
    [201] = "BOOTSTAGE_ID_ACCUM_DM_SPL",
    [202] = "BOOTSTAGE_ACCUM_DM_F",
    [203] = "BOOTSTAGE_ACCUM_DM_R",
    [204] = "BOOTSTAGE_ID_ACCUM_FSP_M",
    [205] = "BOOTSTAGE_ID_ACCUM_FSP_S",
    [206] = "BOOTSTAGE_ID_ACCUM_MMAP_SPI",
    [207] = "BOOTSTAGE_ID_USER",
    [208] = "BOOTSTAGE_ID_ALLOC",
    [300] = "BOOTSTAGE_KERNEL_START",
    [301] = "BOOTSTAGE_KERNEL_END",
};

enum boot_markers {
	BOOTSTAGE_START_UBOOT = 178,
	BOOTSTAGE_START_MCU = 176,
	BOOTSTAGE_BOOTM_HANDOFF = 185,
	BOOTSTAGE_KERNEL_START = 300,
	BOOTSTAGE_KERNEL_END,
};
