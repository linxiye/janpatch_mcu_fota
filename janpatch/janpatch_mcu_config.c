#include "janpatch.h"

struct janpatch_mcu_dev{
    uint8_t check_file;
    uint8_t source_buffer[JANPATCH_MCU_CONFIG_SECTOR_SIZE];
    uint8_t patch_buffer[JANPATCH_MCU_CONFIG_SECTOR_SIZE];
    uint8_t target_buffer[JANPATCH_MCU_CONFIG_SECTOR_SIZE];
#ifdef JANPATCH_MCU_CONFIG_WINDOW_ENABLE
    uint8_t windows_ram[JANPATCH_MCU_CONFIG_WINDOW_SIZE];
    uint32_t windows_ram_used_len;
    JANPATCH_STREAM *target_file; 
    uint32_t windows_flash_write_addr;
#endif
    struct janpatch_mcu_config *config;
};

static struct janpatch_mcu_dev dev;

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
            dev.config->flash_write(dev.target_file->file_address + dev.windows_flash_write_addr, dev.windows_ram, len);
            
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
    uint32_t size = buf_len * buf_size;
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
            dev.config->flash_write(dev.target_file->file_address + dev.windows_flash_write_addr, dev.windows_ram, size);
            
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
    source->file_type = JANPATCH_FILE_SOURCE;
    patch->file_type = JANPATCH_FILE_PATCH;
    target->file_type = JANPATCH_FILE_TARGET;
    ctx.source_buffer.buffer = dev.source_buffer;
    ctx.source_buffer.size = sizeof(dev.source_buffer);
    ctx.patch_buffer.buffer = dev.patch_buffer;
    ctx.patch_buffer.size = sizeof(dev.patch_buffer);
    ctx.target_buffer.buffer = dev.target_buffer;
    ctx.target_buffer.size = sizeof(dev.target_buffer);
    
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
    ctx.fwrite = ctx_cfwrite;
    val = janpatch(ctx, source, patch, target);
    if(val != 0) {
        JANPATCH_ERROR("janpatch failed: %d\r\n", val);
        return val;
    }

    JANPATCH_DEBUG("target file size:%d\r\n", target->file_seek);

    if(target->file_seek > target->file_size){
        JANPATCH_ERROR("target failed over addr target max size: %d\r\n", target->file_size);
        return -1;
    }
#ifdef JANPATCH_MCU_CONFIG_WINDOW_ENABLE
    if((target->file_seek - source->file_size) > JANPATCH_MCU_CONFIG_WINDOW_SIZE)
    {
        JANPATCH_ERROR("window size is too small\r\n");
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
