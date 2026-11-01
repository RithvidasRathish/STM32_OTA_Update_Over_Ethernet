#include "wizchip_conf.h"
#include "wizchip_port.h"
#include "socket.h"
#include "dns/dns.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "main.h"
#include "flash_layout.h"
#include "flash_operations.h"
#include "app_ota.h"

//#define DNS_RESOLVE			   	1  // comment out to use static SERVER IP



/* ================= CONFIG ================= */

#if defined(DNS_RESOLVE)
#define SERVER_PORT        	4242
#define SERVER_HOST        	"tcpbin.com"   // <-- hostname : 4242
#define DNS_SOCKET          6                       // separate socket for DNS
#define DNS_SERVER_IP       {8, 8, 8, 8}            // Google's DNS
uint8_t dns_buffer[512];
static uint8_t dns_ip[4] = DNS_SERVER_IP;
#else
#define SERVER_IP  			{192, 168, 1, 2}
#define SERVER_PORT        	5678
#endif

#define CLIENT_SOCKET      	0
#define CLIENT_PORT 		1234

#define OTA_RX_BUF_SIZE		512
uint32_t CHUNK_SIZE = 512;

static uint8_t server_ip[4];

/* ========================================= */
uint8_t ota_active = 0;

/* OTA state variables */
static uint8_t rx_buf[OTA_RX_BUF_SIZE];
static uint32_t flash_offset = OTA_START_ADDR;
static uint32_t bytes_received = 0;
static uint8_t flash_erased = 0;
static ota_image_hdr_t ota_hdr;
static uint8_t header_received = 0;
static uint8_t ota_session_active = 0;
static uint8_t request_sent = 0;

static uint32_t bytes_in_current_chunk = 0;
static uint32_t total_received  = 0;

#define OTA_HEADER_SIZE sizeof(ota_image_hdr_t)

/* Forward declarations */
static void ota_process_rx(uint8_t *data, uint16_t len);
static void ota_finalize(void);


#if defined(DNS_RESOLVE)
/**
 * @brief Resolve hostname to IP using DNS
 * @retval 0 on success, -1 on failure
 */
int TCP_Client_ResolveDNS(void)
{
	printf("Resolving %s ...\r\n", SERVER_HOST);

	DNS_init(DNS_SOCKET, dns_buffer);

	int8_t result = DNS_run(dns_ip, (uint8_t *)SERVER_HOST, server_ip);

	if (result != 1)
	{
		printf("DNS resolution failed (code=%d)\r\n", result);
		return -1;
	}

	printf("Resolved %s -> %d.%d.%d.%d\r\n", SERVER_HOST,
			server_ip[0], server_ip[1], server_ip[2], server_ip[3]);
	return 0;
}
#endif

static void ota_write (uint32_t addr, uint8_t *buf, size_t len)
{
	HAL_FLASH_Unlock();
	flash_write_bytes(addr, buf, len);
	HAL_FLASH_Lock();
}

void ota_erase_flash()
{
	HAL_FLASH_Unlock();
	flash_erase_ota();
	HAL_FLASH_Lock();
	flash_erased = 1;
}

int8_t OTA_Client_Init(void)
{
    int8_t ret;
    uint8_t status;

    HAL_Delay(20);

    // Force clean state
    close(CLIENT_SOCKET);
    while (getSn_SR(CLIENT_SOCKET) != SOCK_CLOSED) {
        HAL_Delay(10);
    }

    // Re-open socket (use a fixed local port > 1024, or 0 for auto)
    ret = socket(CLIENT_SOCKET, Sn_MR_TCP, CLIENT_PORT, 0);
    if (ret != CLIENT_SOCKET) {
        printf("OTA client: socket() failed, ret=%d\r\n", ret);
        return -1;
    }

    HAL_Delay(50);  // give chip time

    do {
    	status = getSn_SR(CLIENT_SOCKET);
    } while (status != SOCK_INIT);

    printf("Sn_SR = 0x%02X\r\n", status);

#ifdef DNS_RESOLVE
	// Step 1: Resolve the hostname
	if (TCP_Client_ResolveDNS() != 0)
		return -1;
#else
    uint8_t static_ip[4] = SERVER_IP;
    memcpy(server_ip, static_ip, 4);
#endif

    printf("Connecting to server %d.%d.%d.%d:%d ...\r\n",
    		server_ip[0], server_ip[1], server_ip[2], server_ip[3],
           SERVER_PORT);

    int retries = 5;

    do {
    	ret = connect(CLIENT_SOCKET, server_ip, SERVER_PORT);
    	status = getSn_SR(CLIENT_SOCKET);
    	uint32_t max_wait_ms = 5000;
    	uint32_t start = HAL_GetTick();
    	while (((HAL_GetTick() - start) < max_wait_ms) && (status != SOCK_ESTABLISHED))
    	{
    		status = getSn_SR(CLIENT_SOCKET);
    		if (status == SOCK_ESTABLISHED)break;
    	}
    	retries--;
    } while ((retries>0) && (status != SOCK_ESTABLISHED));

    if (status != SOCK_ESTABLISHED){
        printf("Connection timeout\r\n");
        close(CLIENT_SOCKET);
        return -2;
    }

    printf("connected\n");

    return 0;


}

void OTA_Client_Task(void)
{
	int32_t len;
	uint8_t status = getSn_SR(CLIENT_SOCKET);

	switch (status)
	{

	case SOCK_ESTABLISHED:


		if (!request_sent)
		{
			const char req[] = "GET firmware\r\n";
			len = send(CLIENT_SOCKET, (uint8_t*)req, strlen (req));
			if (len>0){
			printf("Sent OTA request\n");
			request_sent = 1;
			}
			else printf ("Sending Error \n");
		}

		uint16_t rx_size = getSn_RX_RSR(CLIENT_SOCKET);
		if (rx_size > 0)
		{
		    if (rx_size > OTA_RX_BUF_SIZE)
		        rx_size = OTA_RX_BUF_SIZE;

		    len = recv(CLIENT_SOCKET, rx_buf, rx_size);
		    if (len > 0)
		    {
		        ota_process_rx(rx_buf, len);

		        bytes_in_current_chunk += len;
		        total_received += len;

		        if (bytes_in_current_chunk >= CHUNK_SIZE)
		        {
			        send(CLIENT_SOCKET, (uint8_t*)"A", 1);   // ACK EVERY recv
			        printf("Sent ACK\n");
		            bytes_in_current_chunk = 0;
		        }
		    }
		}

		else if (len == 0)
		{
		    disconnect(CLIENT_SOCKET);
		}

		break;

	case SOCK_CLOSE_WAIT:
		// Drain remaining bytes
		len = recv(CLIENT_SOCKET, rx_buf, sizeof(rx_buf));
		if (len > 0)
		{
			ota_process_rx(rx_buf, len);
			total_received += len;
		}

		printf("Closing (close-wait) — received %lu bytes\n", total_received);
//		disconnect(CLIENT_SOCKET);
		break;

	case SOCK_CLOSED:
		if (ota_session_active)
		{
			printf("Session finished — total %lu bytes\n", total_received);
			ota_session_active = 0;
		}
		break;

	default:
		// unexpected states → usually safe to close
		if (ota_session_active)
		{
			disconnect(CLIENT_SOCKET);
		}
		break;
	}
}


/* OTA RX PROCESSING                          */

void ota_process_rx(uint8_t *data, uint16_t len)
{
	uint16_t offset = 0;

	/* Erase flash once */
	if (!flash_erased)
	{
		ota_erase_flash();
		flash_erased = 1;
	}

	/* ---------------- Parse OTA header ---------------- */
	if (!header_received)
	{
		uint16_t hdr_bytes_needed = OTA_HEADER_SIZE - bytes_received;

		if (len >= hdr_bytes_needed)
		{
			memcpy(((uint8_t*)&ota_hdr) + bytes_received, data, hdr_bytes_needed);

			/* WRITE HEADER TO FLASH */
			ota_write(flash_offset, data, hdr_bytes_needed);
			flash_offset += hdr_bytes_needed;

			offset += hdr_bytes_needed;
			bytes_received += hdr_bytes_needed;
			header_received = 1;

			printf("OTA header received: magic=0x%08X, size=%lu, version=%lu\n",
					(unsigned int)ota_hdr.magic, ota_hdr.image_size, ota_hdr.version);
		}
		else
		{
			memcpy(((uint8_t*)&ota_hdr) + bytes_received, data, len);

			/* WRITE PARTIAL HEADER TO FLASH */
			ota_write(flash_offset, data, len);
			flash_offset += len;

			bytes_received += len;
			return;
		}
	}


	/* ---------------- Write firmware body ---------------- */
	uint32_t fw_bytes_received = bytes_received - OTA_HEADER_SIZE;
	uint32_t fw_bytes_to_write = len - offset;

	if (fw_bytes_received + fw_bytes_to_write > ota_hdr.image_size)
		fw_bytes_to_write = ota_hdr.image_size - fw_bytes_received;

	if (fw_bytes_to_write > 0)
	{
		ota_write(flash_offset, &data[offset], fw_bytes_to_write);
		flash_offset   += fw_bytes_to_write;
		bytes_received += fw_bytes_to_write;
	}

	printf("Received chunk: %u bytes, total: %lu/%lu\n",
			len,
			bytes_received,
			ota_hdr.image_size + OTA_HEADER_SIZE);

	/* ---------------- Completion ---------------- */
	if (header_received &&
			bytes_received >= ota_hdr.image_size + OTA_HEADER_SIZE)
	{
		printf("OTA upload complete! Firmware bytes: %lu\nRebooting...\n", ota_hdr.image_size);
		ota_finalize();
	}
}


void ota_start(void)
{
	if (W5500_Init() != 0)
		return;

	HAL_Delay(2000);  // let the PHY stabilize

	printf("OTA client Started..\r\n");
	uint8_t retries = 5;
	do {
		if (OTA_Client_Init() == 0)
		{
			ota_active = 1;
			break;
		}
		HAL_Delay(1000);
		retries--;
	} while (retries>0);

}


static void ota_finalize(void)
{
	disconnect(CLIENT_SOCKET);
	close(CLIENT_SOCKET);
	ota_active = 0;
	printf("Check1\n");

	enable_ota_request();   // bootloader flag
	HAL_Delay(200);
	printf("Check2\n");

	NVIC_SystemReset();
//	printf("Check3\n");
}

