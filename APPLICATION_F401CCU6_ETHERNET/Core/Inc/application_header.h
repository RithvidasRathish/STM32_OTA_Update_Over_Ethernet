#ifndef INC_APPLICATION_HEADER_H_
#define INC_APPLICATION_HEADER_H_

#include "flash_layout.h"

#define APP_MAGIC	0xABCDEFAB

typedef struct
{
	uint32_t ota_flag;
    uint32_t magic;
    uint32_t size;      // application size in bytes
    uint32_t crc;       // CRC32 of application
    uint32_t version;
} app_header_t;

#endif
