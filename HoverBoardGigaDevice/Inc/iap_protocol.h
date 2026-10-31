#ifndef IAP_PROTOCOL_H
#define IAP_PROTOCOL_H

#include <stdint.h>

#define IAP_START_FRAME     0xABCEu
#define IAP_HEADER_LEN      6u
#define IAP_CRC_LEN         4u
#define IAP_MAX_PAYLOAD     256u
#define IAP_MAX_FRAME       (IAP_HEADER_LEN + IAP_MAX_PAYLOAD + IAP_CRC_LEN)
#define IAP_MSG_HELLO       0x01u
#define IAP_RUN_ADDR        0x08000201u

static inline uint32_t iap_crc32(const uint8_t *data, uint32_t len)
{
	uint32_t crc = 0xFFFFFFFFu;
	for (uint32_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (int b = 0; b < 8; b++) {
			uint32_t mask = -(crc & 1u);
			crc = (crc >> 1) ^ (0xEDB88320u & mask);
		}
	}
	return ~crc;
}

static inline void iap_enter_from_app(void)
{
	typedef void (*iap_fn)(void);
	__disable_irq();
	((iap_fn)IAP_RUN_ADDR)();
	while (1) {
	}
}

#endif
