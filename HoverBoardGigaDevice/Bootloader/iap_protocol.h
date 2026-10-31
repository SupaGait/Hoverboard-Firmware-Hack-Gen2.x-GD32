#ifndef IAP_PROTOCOL_H
#define IAP_PROTOCOL_H

#include <stdint.h>

#define IAP_START_FRAME     0xABCEu
#define IAP_HEADER_LEN      6u
#define IAP_CRC_LEN         4u
#define IAP_MAX_PAYLOAD     256u
#define IAP_MAX_FRAME       (IAP_HEADER_LEN + IAP_MAX_PAYLOAD + IAP_CRC_LEN)

#define IAP_MSG_HELLO       0x01u
#define IAP_MSG_BEGIN       0x02u
#define IAP_MSG_DATA        0x03u
#define IAP_MSG_COMMIT      0x04u
#define IAP_MSG_ABORT       0x05u
#define IAP_MSG_ACK         0x81u
#define IAP_MSG_NACK        0x82u
#define IAP_MSG_DONE        0x83u

#define IAP_ROLE_SELF       0u
#define IAP_ROLE_SLAVE      1u

#define IAP_NACK_BAD_CRC    1u
#define IAP_NACK_STATE      2u
#define IAP_NACK_TOO_LARGE  3u
#define IAP_NACK_SEQ        4u
#define IAP_NACK_FLASH      5u
#define IAP_NACK_IMAGE_CRC  6u
#define IAP_NACK_ROLE       7u

#define IAP_BL_BASE         0x08000000u
#define IAP_BL_SIZE         0x00001000u
#define IAP_APP_BASE        0x08001000u
/* 64 KiB flash, 4 KiB BL, 1 KiB config page at end → 59 KiB app region. */
#define IAP_APP_MAX_SIZE    0x0000EC00u
#define IAP_APP_META_ADDR   (IAP_APP_BASE + IAP_APP_MAX_SIZE - 8u)
#define IAP_IMAGE_MAX       (IAP_APP_MAX_SIZE - 8u)
#define IAP_CONFIG_PAGE     0x0800FC00u
#define IAP_PAGE_SIZE       0x400u

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

#endif
