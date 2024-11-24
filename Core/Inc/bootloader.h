#ifndef BOOTLOADER_H
#define BOOTLOADER_H
#include "main.h"
#include "stdint.h"
#include "stdbool.h"
#define FIRMWARER_VERSION_NUMBER   FIRM_VER
#define HARDWARER_VERSION_NUMBER   HW_VER_NUMBER
/*size cho boot loader hien dang toi da la 0x8000 (32kb) bat dau tu 0x8000000-0x8008000


    0x8000000 -> 0x8008000  :bootloader    16 pages (32kb)
    0x8008000 -> 0x8021000  :application  50 pages * 2048 = 102,400 byte (100kb) = 0x19000
    0x8021000 -> 0x8023000  :reversed      4 pages
    0x8023000 -> 0x803C000  :fw download  50 pages * 2048 = 102,400 byte (100kb) = 0x19000
		0x803C000 -> 0x803E000	:reversed      4 pages
		0x0803E000 luu data cua application , page 124, page 125 reversed
		0x0803F000 luu info firmware update , page 126, page 127 reversed

*/
#define FLASH_ADDR_FIRMWARE_UPDATE_DOWNLOAD 0x8023000
#define FLASH_SIZE_FRIMWARE_UPDATE_AREA 0x19000
#define FLASH_NUM_PAGE_FRIMWARE_UPDATE_AREA 50

#define FIRMWARE_BASE_ADDR_DOWNLOAD 0x8023000 //=FLASH_ADDR_FIRMWARE_UPDATE_DOWNLOAD

#define FLASH_ADDR_START_APPLICATION 0x8008000		//vung flash sau softdevice (moi sdk size softdevice se khac nhau)
#define FLASH_ADDR_FIRMWARE_UPDATE_INFO 0x0803F000				//vung flash luu config ve update firmware ma application ghi vao




typedef struct __attribute__((packed))			//__attribute__((packed)) noi voi complie k add padding alignment
{
	uint32_t firmwareSize;
	uint32_t isNeedUpdateFirmware;
	uint32_t crc32;
} firmware_update_info_t;


bool firmware_update_checkHaveNewFimrware();
bool firmware_update_updateNewFirmware();
void debugPrint(const char *fmt, ...);
#endif

