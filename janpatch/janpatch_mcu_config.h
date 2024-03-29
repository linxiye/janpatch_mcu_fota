#ifndef __JANPATCH_CONFIG_H
#define __JANPATCH_CONFIG_H

#include "stdio.h"
#include "stdint.h"

#define JANPATCH_MCU_CONFIG_WINDOW_ENABLE

#define JANPATCH_MCU_CONFIG_SECTOR_SIZE         (2048)
#define JANPATCH_MCU_CONFIG_WINDOW_SIZE         (20 * 1024)

#ifndef JANPATCH_DEBUG
#define JANPATCH_DEBUG(...)  printf(__VA_ARGS__)
#endif

#ifndef JANPATCH_ERROR
#define JANPATCH_ERROR(...)  printf(__VA_ARGS__)
#endif


typedef enum {
    JANPATCH_FILE_SOURCE = 0,
    JANPATCH_FILE_PATCH,
    JANPATCH_FILE_TARGET,
}janpatch_file_type;

typedef struct janpatch_file {
	janpatch_file_type  file_type;
    uint32_t            file_address;
    uint32_t            file_seek;
    uint32_t            file_size;      /* target file is max size */
    uint32_t            file_crc32;
}janpatch_file_t;

#define JANPATCH_STREAM janpatch_file_t

struct janpatch_mcu_config{
    uint32_t (*flash_read)(uint32_t address, uint8_t *buf, uint32_t size);
    uint32_t (*flash_write)(uint32_t address, uint8_t *buf, uint32_t size);
    uint32_t (*flash_erase)(uint32_t address);
};

int janpatch_mcu_cinfig_fota(struct janpatch_mcu_config *config, 
                                        janpatch_file_t *source, 
                                        janpatch_file_t *patch,
                                        janpatch_file_t *target);

#endif
