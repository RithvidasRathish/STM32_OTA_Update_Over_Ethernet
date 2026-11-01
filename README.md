# STM32_OTA_Update_Over_Ethernet

Firmware updates over Ethernet on an STM32F401CCU6. The application downloads a new firmware image over the network using a W5500 Ethernet controller, and a bootloader validates and installs it, so no ST-Link is needed for updates.

## Features
- Firmware image downloaded over Ethernet (TCP client on the W5500)
- Update triggered by a button press (interrupt on PA1)
- Separate flash regions for the bootloader, the running application and the downloaded image
- Bootloader installs the new image on the next reset
- Application validation before every jump: header magic, reset vector, image size and CRC
- Clear UART debug messages for each stage and each validation error
- Built with STM32CubeIDE and the STM32 HAL

## Hardware
| Component | Details |
|-----------|---------|
| MCU | STM32F401CCU6 (Black Pill) |
| Ethernet | W5500 module over SPI1 |
| Debug | USART1, 115200 8N1 |
| Status LED | PC13 |
| Update trigger | Button on PA1 |
| Programmer | ST-Link (first flash only) |

## Repository structure
```
APPLICATION_F401CCU6_ETHERNET/   # Application: OTA client over Ethernet
BOOTLOADER_F401CCU6_ETHERNET/    # Bootloader: validation, install and jump
application.bin                  # Application binary used for OTA
```

## How it works
1. On reset, the bootloader starts first and prints `Inside Bootloader!!` over UART.
2. It checks for a pending update request. If there is one, it installs the downloaded image (`Performing OTA...`) and runs it.
3. It validates the application: header magic, reset vector, image size and CRC. If validation fails, the bootloader stays in place and reports the exact error
