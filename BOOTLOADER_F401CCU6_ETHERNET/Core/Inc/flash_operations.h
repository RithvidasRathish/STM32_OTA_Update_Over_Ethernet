#ifndef INC_FLASH_OPERATIONS_H_
#define INC_FLASH_OPERATIONS_H_


uint32_t flash_read_ota_flag(void);
uint32_t flash_erase_app (void);
void flash_write_word(uint32_t addr, uint32_t data);
uint32_t flash_erase_header (void);
uint32_t flash_erase_ota (void);
void flash_write_bytes(uint32_t addr, uint8_t *buf, uint32_t len);


#endif /* INC_FLASH_OPERATIONS_H_ */
