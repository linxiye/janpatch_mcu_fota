#include "janpatch.h"

struct janpatch_mcu_dev{
    uint32_t check_file;
    uint32_t source_buffer[JANPATCH_MCU_CONFIG_SECTOR_SIZE / 4];
    uint32_t patch_buffer[JANPATCH_MCU_CONFIG_SECTOR_SIZE / 4];
    uint32_t target_buffer[JANPATCH_MCU_CONFIG_SECTOR_SIZE / 4];
#ifdef JANPATCH_MCU_CONFIG_WINDOW_ENABLE
    uint32_t windows_ram[JANPATCH_MCU_CONFIG_WINDOW_SIZE / 4];
    uint32_t windows_ram_used_len;
    JANPATCH_STREAM *target_file; 
    uint32_t windows_flash_write_addr;
#endif
    struct janpatch_mcu_config *config;
    uint32_t check_crc;
};

static struct janpatch_mcu_dev dev;

static const uint32_t crc32Table[] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t janpatch_Crc32(uint8_t *buf, uint32_t size)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < size; i++)
    {  
        crc = crc32Table[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}


size_t ctx_fread(void *buf, size_t buf_len, size_t buf_size, JANPATCH_STREAM *p)
{
	uint32_t len;
    uint32_t size = buf_len * buf_size;
    if(p->file_seek + size > p->file_size)
      size = p->file_size - p->file_seek;
	dev.config->flash_read(p->file_address + p->file_seek, buf, size);

    if(dev.check_file == 0) return size;

#ifdef JANPATCH_MCU_CONFIG_WINDOW_ENABLE

    if(p->file_type == JANPATCH_FILE_SOURCE)
    {
        while(p->file_seek > dev.windows_flash_write_addr &&
                dev.windows_ram_used_len > 0)
        {
            if(dev.windows_ram_used_len >= 2048) len = 2048;
            dev.config->flash_erase(dev.target_file->file_address + dev.windows_flash_write_addr);
            dev.config->flash_write(dev.target_file->file_address + dev.windows_flash_write_addr, (uint8_t *)&dev.windows_ram[0], len);
            
            dev.windows_flash_write_addr = dev.windows_flash_write_addr + len;
            dev.windows_ram_used_len = dev.windows_ram_used_len - len;
            for(int i = 0; i < dev.windows_ram_used_len; i++)
            {
                dev.windows_ram[i] = dev.windows_ram[i + len];
            }
            JANPATCH_DEBUG("write:%#X %#X\r\n", dev.windows_flash_write_addr, dev.windows_ram_used_len);
        }
    }
#endif
	return size;
}

size_t ctx_fwrite(const void *buf, size_t buf_len, size_t buf_size, JANPATCH_STREAM *p)
{
	uint8_t *data = (uint8_t *)buf;
    uint32_t size = buf_len * buf_size;
    if(p->file_seek + size > p->file_size)
      size = p->file_size - p->file_seek;

#ifdef JANPATCH_MCU_CONFIG_WINDOW_ENABLE
        for(int i = 0; i < size; i++)
        {
            dev.windows_ram[dev.windows_ram_used_len + i] = data[i];
        }
        dev.windows_ram_used_len = dev.windows_ram_used_len + size;
        JANPATCH_DEBUG("save:%#X %#X\r\n", dev.windows_flash_write_addr, dev.windows_ram_used_len);
#else
        dev.config->flash_erase(p->file_address + p->file_seek);
        dev.config->flash_write(p->file_address + p->file_seek, (uint8_t *)buf, size);
#endif
    
    p->file_seek = p->file_seek + size;
    return size;
}

size_t ctx_cfwrite(const void *buf, size_t buf_len, size_t buf_size, JANPATCH_STREAM *p)
{
    uint8_t *data = (uint8_t *)buf;
	uint32_t size = buf_len * buf_size;
    for (uint32_t i = 0; i < size; i++)
    {  
        dev.check_crc = crc32Table[(dev.check_crc ^ data[i]) & 0xff] ^ (dev.check_crc >> 8);
    }
    p->file_seek = p->file_seek + size;
    return size;
}

int ctx_fseek(JANPATCH_STREAM *p, long int seek, int type)
{
    if(type == SEEK_SET)
            p->file_seek = 0 + seek;
    else if(type == SEEK_CUR)
            p->file_seek = p->file_seek + seek;
    else if(type == SEEK_END)
            p->file_seek = p->file_size - seek;
    if(p->file_seek > p->file_size)
        return -1;
	return 0;
}

long ctx_ftell(JANPATCH_STREAM *p)
{
    return p->file_seek;
}

void ctx_progress(uint8_t val)
{
	uint32_t size;

    if(dev.check_file == 0) {
        JANPATCH_DEBUG("progress:%d\r\n", val);
        return;
    }

#ifdef JANPATCH_MCU_CONFIG_WINDOW_ENABLE

	if(val == 100)
	{
		while(dev.target_file->file_seek > dev.windows_flash_write_addr)
		{
            if(dev.windows_ram_used_len >= 2048) size = 2048;
            else size = dev.windows_ram_used_len;
            dev.config->flash_erase(dev.target_file->file_address + dev.windows_flash_write_addr);
            dev.config->flash_write(dev.target_file->file_address + dev.windows_flash_write_addr, (uint8_t *)&dev.windows_ram[0], size);
            
            dev.windows_flash_write_addr = dev.windows_flash_write_addr + size;
            dev.windows_ram_used_len -= size;

      	    for(int i = 0; i < dev.windows_ram_used_len; i++)
            {
                dev.windows_ram[i] = dev.windows_ram[i + size];
            }
            JANPATCH_DEBUG("save:%#X %#X\r\n", dev.windows_flash_write_addr, dev.windows_ram_used_len);
		}	
	}
#endif
	JANPATCH_DEBUG("progress:%d\r\n", val);
}

static janpatch_ctx ctx = {
    .fread = ctx_fread,
    .fwrite = ctx_fwrite,
    .fseek = ctx_fseek,
    .ftell = ctx_ftell,
    .progress = ctx_progress,
};

int janpatch_mcu_cinfig_fota(struct janpatch_mcu_config *config, 
                                        janpatch_file_t *source, 
                                        janpatch_file_t *patch,
                                        janpatch_file_t *target)
{
    int val;
    uint8_t temp[24];
    /* 更新类型，赋予缓存及缓存大小 */
    source->file_type = JANPATCH_FILE_SOURCE;
    patch->file_type = JANPATCH_FILE_PATCH;
    target->file_type = JANPATCH_FILE_TARGET;
    ctx.source_buffer.buffer = (uint8_t *)&dev.source_buffer[0];
    ctx.source_buffer.size = sizeof(dev.source_buffer);
    ctx.patch_buffer.buffer = (uint8_t *)&dev.patch_buffer[0];
    ctx.patch_buffer.size = sizeof(dev.patch_buffer);
    ctx.target_buffer.buffer = (uint8_t *)&dev.target_buffer[0];
    ctx.target_buffer.size = sizeof(dev.target_buffer);
    
    /* 从patch头部获取各个文件信息 */
    config->flash_read(patch->file_address, temp, sizeof(temp));
    source->file_size = temp[0] << 24 | temp[1] << 16 | temp[2] << 8 | temp[3];
    source->file_crc32 = temp[4] << 24 | temp[5] << 16 | temp[6] << 8 | temp[7];
    target->file_size = temp[8] << 24 | temp[9] << 16 | temp[10] << 8 | temp[11];
    target->file_crc32 = temp[12] << 24 | temp[13] << 16 | temp[14] << 8 | temp[15];
    patch->file_size = temp[16] << 24 | temp[17] << 16 | temp[18] << 8 | temp[19];
    patch->file_crc32 = temp[20] << 24 | temp[21] << 16 | temp[22] << 8 | temp[23];
    patch->file_address = patch->file_address + 24;

    JANPATCH_DEBUG("source file size    :0x%08X\r\n", source->file_size);
    JANPATCH_DEBUG("source file crc     :0x%08X\r\n", source->file_crc32);
    JANPATCH_DEBUG("target file size    :0x%08X\r\n", target->file_size);
    JANPATCH_DEBUG("target file crc     :0x%08X\r\n", target->file_crc32);
    JANPATCH_DEBUG("patch file size     :0x%08X\r\n", patch->file_size);
    JANPATCH_DEBUG("patch file crc      :0x%08X\r\n", patch->file_crc32);    

    dev.config = config;
#ifdef JANPATCH_MCU_CONFIG_WINDOW_ENABLE
    dev.target_file = target;
#else
    if(source->file_address == target->file_address){
        JANPATCH_ERROR("janpatch window disable and source address is the same as target address\r\n");
        JANPATCH_ERROR("please enable window or change address\r\n");
        return -1;
    }
#endif

    /* 校验 */
    dev.check_crc = 0xFFFFFFFF;
    ctx.fwrite = ctx_cfwrite;
    val = janpatch(ctx, source, patch, target);
    if(val != 0) {
        JANPATCH_ERROR("janpatch failed: %d\r\n", val);
        return val;
    }
    dev.check_crc = dev.check_crc ^ 0xFFFFFFFF;
    JANPATCH_DEBUG("check crc      :0x%08X\r\n", dev.check_crc);    
    if(dev.check_crc != target->file_crc32){
        JANPATCH_ERROR("target failed crc error: 0x%08X 0x%08X\r\n", dev.check_crc, target->file_crc32);
        return -1;
    }


    JANPATCH_DEBUG("target file size:%d\r\n", target->file_seek);

    if(target->file_seek > target->file_size){
        JANPATCH_ERROR("target failed over addr target max size: %d\r\n", target->file_size);
        return -1;
    }
#ifdef JANPATCH_MCU_CONFIG_WINDOW_ENABLE
		uint32_t packsize = (target->file_seek > source->file_size) ? (target->file_seek - source->file_size) : (source->file_size - target->file_seek);
    if(packsize > JANPATCH_MCU_CONFIG_WINDOW_SIZE)
    {
        JANPATCH_ERROR("window size is too small %d\r\n", packsize);
        return -1;
    }
#endif

    dev.check_file = 1;
    ctx.fwrite = ctx_fwrite;
    val = janpatch(ctx, source, patch, target);
    if(val != 0) {
        JANPATCH_ERROR("janpatch failed: %d\r\n", val);
        return val;
    }



    return 0;
}
