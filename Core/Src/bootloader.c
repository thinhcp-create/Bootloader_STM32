#include "bootloader.h"
#include "FLASH_PAGE_F1.h"
#include "stdbool.h"
#include "main.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"
extern CRC_HandleTypeDef hcrc;
extern UART_HandleTypeDef huart1;


void debugPrint(const char *fmt, ...)
{
	char buff[300] = {0};
	char buff2[320] = {0};
	va_list arg;
	va_start(arg, fmt);
	vsprintf(buff, fmt, arg);
	va_end(arg);
	sprintf(buff2,"@>%03d%s",strlen(buff),buff);
	HAL_UART_Transmit(&huart1,(uint8_t*)buff2,strlen(buff2),100);
}

#define FIRMWARE_UPDATE_DGB debugPrint

#define NRF_SUCCESS 0
#define CODE_PAGE_SIZE FLASH_PAGE_SIZE

firmware_update_info_t fwUpdateInfo = {0};
////for test
//{
//	.firmwareSize = 0x5678,
////	.isNeedUpdateFirmware = 0,
////	.crc32 = 0x92A44637
//	.isNeedUpdateFirmware = 1,
//	.crc32 = 0x96655B80
//};
uint8_t buffer_page_read_write[CODE_PAGE_SIZE];//buffer to read write flash, assurance roundup 4 byte unit write flash
static uint32_t round_up_u32(uint32_t len)
{
    if (len % sizeof(uint32_t))
    {
        return (len + sizeof(uint32_t) - (len % sizeof(uint32_t)));
    }

    return len;
}
void firmware_update_log_hex(char* tag,void* buf, uint32_t len)
{
    debugPrint("%s\n",tag);
    for(int i = 0; i < len; i++)
    {
    	debugPrint("%02X ", *((uint8_t*)buf + i));
    }
    debugPrint("\n");
}
void firmware_update_commitStateFirmware(bool isSuccess)
{
		fwUpdateInfo.isNeedUpdateFirmware = false;
		uint32_t crc32 = HAL_CRC_Calculate(&hcrc, (uint32_t*)&fwUpdateInfo, 2);
//		uint32_t crc32 = crc32_compute((uint8_t*)&fwUpdateInfo,sizeof(firmware_update_info_t) - 4, NULL);
		fwUpdateInfo.crc32 = crc32;
		memset(buffer_page_read_write, 0, CODE_PAGE_SIZE);
		memcpy(buffer_page_read_write, &fwUpdateInfo, sizeof(fwUpdateInfo));
		FLASH_PageErase(FLASH_ADDR_FIRMWARE_UPDATE_INFO);
		Flash_Write_Data(FLASH_ADDR_FIRMWARE_UPDATE_INFO, (uint32_t*)buffer_page_read_write,3);
//		fmc_erase_page_address(FLASH_ADDR_FIRMWARE_UPDATE_INFO);
//		fmc_program(FLASH_ADDR_FIRMWARE_UPDATE_INFO, (uint32_t*)buffer_page_read_write, round_up_u32(sizeof(fwUpdateInfo))/4);

}
bool firmware_update_checkHaveNewFimrware()
{
//		memset(buffer_page_read_write, 0, CODE_PAGE_SIZE);
//		memcpy(buffer_page_read_write, &fwUpdateInfo, sizeof(fwUpdateInfo));
//		user_flash_erase(FLASH_ADDR_FIRMWARE_UPDATE_INFO, 1);
//		user_flash_writeToFlash(FLASH_ADDR_FIRMWARE_UPDATE_INFO, (uint8_t*)buffer_page_read_write, round_up_u32(sizeof(fwUpdateInfo))/4);

		memcpy((void*)&fwUpdateInfo, (void*)FLASH_ADDR_FIRMWARE_UPDATE_INFO, sizeof(fwUpdateInfo));
		FIRMWARE_UPDATE_DGB("firmware info size %d: crc - 0x%08X\n", sizeof(firmware_update_info_t), fwUpdateInfo.crc32);
//		NRF_LOG_HEXDUMP_INFO(&fwUpdateInfo, sizeof(firmware_update_info_t));
		uint32_t crc32 = HAL_CRC_Calculate(&hcrc, (uint32_t*)&fwUpdateInfo, 2);
//		uint32_t crc32 = crc32_compute((uint8_t*)&fwUpdateInfo,sizeof(firmware_update_info_t) - 4, NULL);
		FIRMWARE_UPDATE_DGB("crc cal: 0x%08X\n", crc32);
		if(fwUpdateInfo.crc32 == 0xFFFFFFFF || fwUpdateInfo.crc32 != crc32)
		{
				FIRMWARE_UPDATE_DGB("fw info empty\n");
		}
		else
		{
				if(fwUpdateInfo.isNeedUpdateFirmware != 0)
				{
						FIRMWARE_UPDATE_DGB("Need update new firmware size %d byte\n",fwUpdateInfo.firmwareSize);
						return true;
				}
		}
		FIRMWARE_UPDATE_DGB("No need update firmware\n");
		FIRMWARE_UPDATE_DGB("Current firmware size %d byte\n",fwUpdateInfo.firmwareSize);
		return false;
}


bool firmware_update_updateNewFirmware()
{
		uint32_t firmware_size = fwUpdateInfo.firmwareSize;
		uint32_t num_page_need_erase;
		uint32_t firmware_bytes_left = firmware_size % CODE_PAGE_SIZE;
		if(firmware_bytes_left == 0)
		{
				num_page_need_erase = firmware_size/CODE_PAGE_SIZE;
		}
		else
		{
				num_page_need_erase = 1 + firmware_size/CODE_PAGE_SIZE;
		}

		uint32_t dst_addr = FLASH_ADDR_START_APPLICATION;
		uint32_t src_addr = FLASH_ADDR_FIRMWARE_UPDATE_DOWNLOAD;


		FIRMWARE_UPDATE_DGB("Firmware size: %d bytes, num page erase: %d\n",firmware_size, num_page_need_erase);

		FIRMWARE_UPDATE_DGB("src_start_addr: 0x%08X, dst_start_addr: 0x%08X\n",src_addr,dst_addr);

		for(uint32_t page_idx = 0; page_idx < num_page_need_erase; page_idx++)
		{
				uint32_t size_copy = CODE_PAGE_SIZE;
				FLASH_PageErase(dst_addr);
				if((page_idx	== num_page_need_erase - 1) && firmware_bytes_left > 0)
				{
						size_copy = round_up_u32(firmware_bytes_left);
				}
				FIRMWARE_UPDATE_DGB("copy from 0x%08X to 0x%08X with %d bytes\n",src_addr, dst_addr, size_copy);
				memcpy(buffer_page_read_write, (void*)src_addr, size_copy);
				Flash_Write_Array(dst_addr, (uint8_t*)buffer_page_read_write, size_copy);

				dst_addr = dst_addr + CODE_PAGE_SIZE;
				src_addr = src_addr + CODE_PAGE_SIZE;
		}

		//calculate checksum or hash to verify
		dst_addr = FLASH_ADDR_START_APPLICATION;
//		sha256_context_t ctx;
//		sha256_init(&ctx);
//		if(sha256_update(&ctx, (uint8_t*)dst_addr, firmware_size) != NRF_SUCCESS)
//		{
//			goto error1;
//		}
//		uint8_t hash256_cal[32];
//		if(sha256_final(&ctx, hash256_cal, 1) != NRF_SUCCESS)
//		{
//			goto error1;
//		}
//		firmware_update_log_hex(TAG,hash256_cal, 32);
//		if(memcmp(fwUpdateInfo.hash256Firmware, hash256_cal, 32) != 0)
//		{
//				FIRMWARE_UPDATE_DGB("hash not match\n");
//				return false;
//		}
//		FIRMWARE_UPDATE_DGB("hash check OK\n");
		firmware_update_commitStateFirmware(true);
		return true;
//		error1:
//			FIRMWARE_UPDATE_DGB("Something error!!!\n");
//
//		//return false if verify fails or any error
//		return false;
//
}

