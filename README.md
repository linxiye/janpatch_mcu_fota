# janpatch_mcu_fota

    本代码是janpatch在MCU使用的Demo例程，janpatch源码:https://github.com/janjongboom/janpatch

## Janpatch 差分算法

* 这是一个可供MCU差分升级算法，在PC中可使用jdiff进行补丁生成，单片机里可使用此算法对旧程序打补丁生成新程序。
janpatch占用非常小，仅占用2.1KB Flash，最少占用 扇区*3 的RAM，非常适合在MCU上使用。默认扇区为2KB，可在janpatch_mcu_config.h
中修改如下宏定义

```
    #define JANPATCH_MCU_CONFIG_SECTOR_SIZE         (2048)
```

* janpatch_mcu_config文件是专门封装给MCU使用的，原先的janpatch代码是Linux代码，读写接口用的标准文件接口，经过封装后，只需简单的配置即可进行差分升级。下面为janpatch使用例子：

```
    /* 旧代码文件描述 （janpatch\demo\blinky-k64f-old.bin） */
    static janpatch_file_t source = {
    .file_address = 0x08020000,     /* 旧代码烧录地址 */
    .file_size = 47352,             /* 旧代码大小 */
    };

    /* 补丁文件描述 （janpatch\demo\blinky-k64f.bin） */
    static janpatch_file_t patch = {
    .file_address = 0x08030000,     /* 补丁烧录地址 */
    .file_size = 8580,              /* 补丁大小 */
    };

    static janpatch_file_t target = {
    .file_address = 0x08010000,     /* 新代码烧录地址 */
    .file_size = 64 * 1024,         /* 新代码最大大小 */
    };

    /* Flash 读、写、擦除扇区操作函数 */
    static struct janpatch_mcu_config janpatch_mcu_config = {
    .flash_erase = Flash_Erase,
    .flash_read = Flash_Read,
    .flash_write = Flash_Write,
    };

    /* 执行函数 */
    janpatch_mcu_cinfig_fota(&janpatch_mcu_config, &source, &patch, &target);

```

* 若Flash过小并RAM有冗余,可以在janpatch_mcu_config.h使能如下宏定义：

```
#define JANPATCH_MCU_CONFIG_WINDOW_ENABLE
```

* 此时target地址与source地址可以相同，但会使用额外RAM,并且差分新程序与差分旧程序相差不能超过额外分配大小的RAM，额外使用RAM由janpatch_mcu_config.h如下宏定义：

```
#define JANPATCH_MCU_CONFIG_WINDOW_SIZE         (20 * 1024)
```

* 添加CRC检验内容，原本的janpatch只是单纯使用jdiff的补丁包生成新程序而已，并不会校验文件完整性，很容易导致补丁包使用其他旧程序与新程序生成的，设备端旧代码打上补丁包后发现程序运行不了，因此这里我添加了文件CRC校验。上位机在jdiff补丁包基础上加上校验头部信息，保证MCU打上补丁后程序完整。上位机源码:https://github.com/linxiye/JFoatTool
