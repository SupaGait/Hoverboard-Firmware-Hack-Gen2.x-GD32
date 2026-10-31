/**
 * UART IAP bootloader for CarrierHasArrived board B (GD32F130).
 * 4 KiB at 0x08000000. App at 0x08001000.
 *
 * Host UART: USART0 on PB6/PB7 (ESP32).
 * Slave relay (master builds): USART1 on PA2/PA3.
 * Slave builds (-DIAP_USE_USART1): host is USART1 only.
 */
#include <stdint.h>
#include <string.h>
#include "iap_protocol.h"

#define RCU_AHBEN     (*(volatile uint32_t *)0x40021014u)
#define RCU_APB2EN    (*(volatile uint32_t *)0x40021018u)
#define RCU_APB1EN    (*(volatile uint32_t *)0x4002101Cu)

#define GPIOA_BASE    0x48000000u
#define GPIOB_BASE    0x48000400u
#define GPIO_MODER(b) (*(volatile uint32_t *)((b) + 0x00u))
#define GPIO_OTYPER(b) (*(volatile uint32_t *)((b) + 0x04u))
#define GPIO_PUPDR(b) (*(volatile uint32_t *)((b) + 0x0Cu))
#define GPIO_AFRL(b)  (*(volatile uint32_t *)((b) + 0x20u))

#define USART0_BASE   0x40013800u
#define USART1_BASE   0x40004400u
#define USART_CTL0(u) (*(volatile uint32_t *)((u) + 0x00u))
#define USART_BAUD(u) (*(volatile uint32_t *)((u) + 0x0Cu))
#define USART_STAT(u) (*(volatile uint32_t *)((u) + 0x1Cu))
#define USART_RDATA(u) (*(volatile uint32_t *)((u) + 0x24u))
#define USART_TDATA(u) (*(volatile uint32_t *)((u) + 0x28u))

#define FMC_KEY       (*(volatile uint32_t *)0x40022004u)
#define FMC_STAT      (*(volatile uint32_t *)0x4002200Cu)
#define FMC_CTL       (*(volatile uint32_t *)0x40022010u)
#define FMC_ADDR      (*(volatile uint32_t *)0x40022014u)
#define FMC_KEY1      0x45670123u
#define FMC_KEY2      0xCDEF89ABu

#ifdef IAP_USE_USART1
#define HOST_USART USART1_BASE
#else
#define HOST_USART USART0_BASE
#endif

enum { ST_IDLE = 0, ST_READY, ST_PROGRAM, ST_RELAY };

static uint8_t g_rx[IAP_MAX_FRAME];
static uint16_t g_rx_n;
static uint8_t g_prev, g_sync, g_state, g_expect_seq;
static uint32_t g_img_size, g_img_crc, g_written, g_run_crc;

static void usart_init(uint32_t usart)
{
  if (usart == USART0_BASE) {
    RCU_AHBEN |= (1u << 18);  /* GPIOB */
    RCU_APB2EN |= (1u << 14); /* USART0 */
    /* PB6/PB7 AF0 */
    uint32_t moder = GPIO_MODER(GPIOB_BASE);
    moder &= ~((3u << 12) | (3u << 14));
    moder |= (2u << 12) | (2u << 14);
    GPIO_MODER(GPIOB_BASE) = moder;
    GPIO_AFRL(GPIOB_BASE) &= ~0xFF000000u; /* AF0 */
    GPIO_PUPDR(GPIOB_BASE) |= (1u << 14);  /* PB7 pull-up */
  } else {
    RCU_AHBEN |= (1u << 17);  /* GPIOA */
    RCU_APB1EN |= (1u << 17); /* USART1 */
    /* PA2/PA3 AF1 */
    uint32_t moder = GPIO_MODER(GPIOA_BASE);
    moder &= ~((3u << 4) | (3u << 6));
    moder |= (2u << 4) | (2u << 6);
    GPIO_MODER(GPIOA_BASE) = moder;
    uint32_t afr = GPIO_AFRL(GPIOA_BASE);
    afr &= ~0x0000FF00u;
    afr |= 0x00001100u; /* AF1 */
    GPIO_AFRL(GPIOA_BASE) = afr;
    GPIO_PUPDR(GPIOA_BASE) |= (1u << 6);
  }
  /* 8 MHz IRC / 115200 → mantissa 69, frac 7 */
  USART_BAUD(usart) = 0x457u;
  USART_CTL0(usart) = (1u << 0) | (1u << 2) | (1u << 3); /* UEN|REN|TEN */
}

static void host_putc(uint8_t c)
{
  while ((USART_STAT(HOST_USART) & (1u << 7)) == 0) {
  }
  USART_TDATA(HOST_USART) = c;
}

static int host_getc(uint8_t *c)
{
  if ((USART_STAT(HOST_USART) & (1u << 5)) == 0) {
    return 0;
  }
  *c = (uint8_t)USART_RDATA(HOST_USART);
  return 1;
}

#ifndef IAP_USE_USART1
static void slave_putc(uint8_t c)
{
  while ((USART_STAT(USART1_BASE) & (1u << 7)) == 0) {
  }
  USART_TDATA(USART1_BASE) = c;
}

static int slave_getc(uint8_t *c)
{
  if ((USART_STAT(USART1_BASE) & (1u << 5)) == 0) {
    return 0;
  }
  *c = (uint8_t)USART_RDATA(USART1_BASE);
  return 1;
}
#endif

static void flash_unlock(void)
{
  if (FMC_CTL & (1u << 7)) {
    FMC_KEY = FMC_KEY1;
    FMC_KEY = FMC_KEY2;
  }
}

static void flash_lock(void)
{
  FMC_CTL |= (1u << 7);
}

static void flash_wait(void)
{
  while (FMC_STAT & 1u) {
  }
}

static int flash_erase_page(uint32_t addr)
{
  flash_wait();
  FMC_CTL |= (1u << 1); /* PER */
  FMC_ADDR = addr;
  FMC_CTL |= (1u << 6); /* START */
  flash_wait();
  FMC_CTL &= ~(1u << 1);
  return 0;
}

static int flash_program_word(uint32_t addr, uint32_t data)
{
  flash_wait();
  FMC_CTL |= (1u << 0); /* PG */
  *(volatile uint32_t *)addr = data;
  flash_wait();
  FMC_CTL &= ~(1u << 0);
  return (*(volatile uint32_t *)addr == data) ? 0 : -1;
}

static int erase_app(void)
{
  flash_unlock();
  for (uint32_t a = IAP_APP_BASE; a < IAP_APP_BASE + IAP_APP_MAX_SIZE; a += IAP_PAGE_SIZE) {
    flash_erase_page(a);
  }
  flash_lock();
  return 0;
}

static int program_bytes(uint32_t addr, const uint8_t *data, uint32_t len)
{
  flash_unlock();
  uint32_t i = 0;
  while (i < len) {
    uint32_t w = 0xFFFFFFFFu;
    uint32_t n = len - i;
    if (n > 4) {
      n = 4;
    }
    memcpy(&w, data + i, n);
    if (flash_program_word(addr + i, w) != 0) {
      flash_lock();
      return -1;
    }
    i += 4;
  }
  flash_lock();
  return 0;
}

static void send_frame_to(void (*putc)(uint8_t), uint8_t msg, uint8_t seq,
                          const uint8_t *payload, uint16_t len)
{
  uint8_t buf[IAP_MAX_FRAME];
  buf[0] = (uint8_t)(IAP_START_FRAME & 0xFF);
  buf[1] = (uint8_t)(IAP_START_FRAME >> 8);
  buf[2] = msg;
  buf[3] = seq;
  buf[4] = (uint8_t)(len & 0xFF);
  buf[5] = (uint8_t)(len >> 8);
  if (len && payload) {
    memcpy(&buf[6], payload, len);
  }
  uint32_t crc = iap_crc32(buf, IAP_HEADER_LEN + len);
  memcpy(&buf[IAP_HEADER_LEN + len], &crc, 4);
  uint16_t total = (uint16_t)(IAP_HEADER_LEN + len + IAP_CRC_LEN);
  for (uint16_t i = 0; i < total; i++) {
    putc(buf[i]);
  }
}

static void send_ack(uint8_t seq) { send_frame_to(host_putc, IAP_MSG_ACK, seq, 0, 0); }
static void send_nack(uint8_t seq, uint8_t code)
{
  send_frame_to(host_putc, IAP_MSG_NACK, seq, &code, 1);
}
static void send_done(uint8_t seq) { send_frame_to(host_putc, IAP_MSG_DONE, seq, 0, 0); }

static int app_valid(void)
{
  uint32_t msp = *(volatile uint32_t *)IAP_APP_BASE;
  uint32_t rst = *(volatile uint32_t *)(IAP_APP_BASE + 4u);
  /* ST-Link programs the image only. The CRC trailer at the end of the region
   * is left untouched, so a sane vector table is what decides the app can run. */
  if (msp < 0x20000000u || msp > 0x20002000u) {
    return 0;
  }
  if ((rst & 1u) == 0u) {
    return 0;
  }
  if (rst < IAP_APP_BASE || rst >= IAP_APP_BASE + IAP_APP_MAX_SIZE) {
    return 0;
  }
  return 1;
}

static void jump_to_app(void)
{
  uint32_t msp = *(volatile uint32_t *)IAP_APP_BASE;
  uint32_t rv = *(volatile uint32_t *)(IAP_APP_BASE + 4u);

  __asm volatile("cpsid i" ::: "memory");
  *((volatile uint32_t *)0xE000E010u) = 0u; /* SysTick->CTRL */
  *((volatile uint32_t *)0xE000E018u) = 0u; /* SysTick->VAL */
  for (uint32_t i = 0; i < 8u; i++) {
    *((volatile uint32_t *)(0xE000E180u + 4u * i)) = 0xFFFFFFFFu; /* NVIC->ICER */
    *((volatile uint32_t *)(0xE000E280u + 4u * i)) = 0xFFFFFFFFu; /* NVIC->ICPR */
  }
  *((volatile uint32_t *)0xE000ED08u) = IAP_APP_BASE; /* VTOR */
  __asm volatile(
      "dsb\n"
      "isb\n"
      "msr msp, %0\n"
      "dsb\n"
      "isb\n"
      "cpsie i\n"
      "bx %1\n"
      :
      : "r"(msp), "r"(rv)
      : "memory");
  while (1) {
  }
}

#ifndef IAP_USE_USART1
/* Wait for one IAP frame from slave; copy raw bytes to out, return length or 0. */
static uint16_t slave_read_frame(uint8_t *out, uint32_t timeout)
{
  uint8_t prev = 0, sync = 0;
  uint16_t n = 0;
  while (timeout--) {
    uint8_t c;
    if (!slave_getc(&c)) {
      continue;
    }
    if (!sync) {
      uint16_t start = ((uint16_t)c << 8) | prev;
      prev = c;
      if (start == IAP_START_FRAME) {
        out[0] = (uint8_t)(IAP_START_FRAME & 0xFF);
        out[1] = (uint8_t)(IAP_START_FRAME >> 8);
        n = 2;
        sync = 1;
      }
      continue;
    }
    out[n++] = c;
    prev = c;
    if (n < IAP_HEADER_LEN) {
      continue;
    }
    uint16_t plen = (uint16_t)out[4] | ((uint16_t)out[5] << 8);
    if (plen > IAP_MAX_PAYLOAD) {
      return 0;
    }
    uint16_t total = (uint16_t)(IAP_HEADER_LEN + plen + IAP_CRC_LEN);
    if (n >= total) {
      return total;
    }
  }
  return 0;
}

static int relay_to_slave(uint8_t msg, uint8_t seq, const uint8_t *payload, uint16_t len)
{
  send_frame_to(slave_putc, msg, seq, payload, len);
  uint8_t resp[IAP_MAX_FRAME];
  uint16_t rn = slave_read_frame(resp, 2000000u);
  if (rn == 0) {
    return -1;
  }
  for (uint16_t i = 0; i < rn; i++) {
    host_putc(resp[i]);
  }
  return 0;
}
#endif

static void handle_frame(uint8_t msg, uint8_t seq, const uint8_t *payload, uint16_t len)
{
  switch (msg) {
  case IAP_MSG_HELLO:
    g_state = ST_READY;
    send_ack(seq);
    break;
  case IAP_MSG_BEGIN:
    if (g_state < ST_READY || len < 9) {
      send_nack(seq, IAP_NACK_STATE);
      return;
    }
    g_img_size = (uint32_t)payload[0] | ((uint32_t)payload[1] << 8) |
                 ((uint32_t)payload[2] << 16) | ((uint32_t)payload[3] << 24);
    g_img_crc = (uint32_t)payload[4] | ((uint32_t)payload[5] << 8) |
                ((uint32_t)payload[6] << 16) | ((uint32_t)payload[7] << 24);
#ifndef IAP_USE_USART1
    if (payload[8] == IAP_ROLE_SLAVE) {
      usart_init(USART1_BASE);
      if (relay_to_slave(IAP_MSG_HELLO, 0, 0, 0) != 0) {
        send_nack(seq, IAP_NACK_ROLE);
        return;
      }
      /* Rewrite BEGIN as role=self for the slave. */
      uint8_t begin_self[9];
      memcpy(begin_self, payload, 9);
      begin_self[8] = IAP_ROLE_SELF;
      if (relay_to_slave(IAP_MSG_BEGIN, seq, begin_self, 9) != 0) {
        send_nack(seq, IAP_NACK_ROLE);
        return;
      }
      g_state = ST_RELAY;
      return;
    }
#endif
    if (payload[8] != IAP_ROLE_SELF) {
      send_nack(seq, IAP_NACK_ROLE);
      return;
    }
    if (g_img_size == 0 || g_img_size > IAP_IMAGE_MAX) {
      send_nack(seq, IAP_NACK_TOO_LARGE);
      return;
    }
    erase_app();
    g_written = 0;
    g_expect_seq = 0;
    g_run_crc = 0xFFFFFFFFu;
    g_state = ST_PROGRAM;
    send_ack(seq);
    break;
  case IAP_MSG_DATA:
#ifndef IAP_USE_USART1
    if (g_state == ST_RELAY) {
      relay_to_slave(msg, seq, payload, len);
      return;
    }
#endif
    if (g_state != ST_PROGRAM || seq != g_expect_seq) {
      send_nack(seq, g_state != ST_PROGRAM ? IAP_NACK_STATE : IAP_NACK_SEQ);
      return;
    }
    if (g_written + len > g_img_size ||
        program_bytes(IAP_APP_BASE + g_written, payload, len) != 0) {
      send_nack(seq, IAP_NACK_FLASH);
      return;
    }
    for (uint16_t i = 0; i < len; i++) {
      g_run_crc ^= payload[i];
      for (int b = 0; b < 8; b++) {
        uint32_t mask = -(g_run_crc & 1u);
        g_run_crc = (g_run_crc >> 1) ^ (0xEDB88320u & mask);
      }
    }
    g_written += len;
    g_expect_seq++;
    send_ack(seq);
    break;
  case IAP_MSG_COMMIT:
#ifndef IAP_USE_USART1
    if (g_state == ST_RELAY) {
      relay_to_slave(msg, seq, payload, len);
      g_state = ST_READY;
      return;
    }
#endif
    if (g_state != ST_PROGRAM || g_written != g_img_size) {
      send_nack(seq, IAP_NACK_STATE);
      return;
    }
    g_run_crc = ~g_run_crc;
    if (g_run_crc != g_img_crc) {
      send_nack(seq, IAP_NACK_IMAGE_CRC);
      return;
    }
    {
      uint8_t meta[8];
      memcpy(meta, &g_img_size, 4);
      memcpy(meta + 4, &g_img_crc, 4);
      if (program_bytes(IAP_APP_META_ADDR, meta, 8) != 0) {
        send_nack(seq, IAP_NACK_FLASH);
        return;
      }
    }
    send_done(seq);
    for (volatile uint32_t d = 0; d < 50000u; d++) {
    }
    jump_to_app();
    break;
  case IAP_MSG_ABORT:
#ifndef IAP_USE_USART1
    if (g_state == ST_RELAY) {
      relay_to_slave(msg, seq, payload, len);
    }
#endif
    g_state = ST_READY;
    send_ack(seq);
    break;
  default:
    send_nack(seq, IAP_NACK_STATE);
    break;
  }
}

static void feed_byte(uint8_t byte)
{
  if (!g_sync) {
    uint16_t start = ((uint16_t)byte << 8) | g_prev;
    g_prev = byte;
    if (start == IAP_START_FRAME) {
      g_rx[0] = (uint8_t)(IAP_START_FRAME & 0xFF);
      g_rx[1] = (uint8_t)(IAP_START_FRAME >> 8);
      g_rx_n = 2;
      g_sync = 1;
    }
    return;
  }
  if (g_rx_n >= IAP_MAX_FRAME) {
    g_sync = 0;
    g_rx_n = 0;
    g_prev = byte;
    return;
  }
  g_rx[g_rx_n++] = byte;
  g_prev = byte;
  if (g_rx_n < IAP_HEADER_LEN) {
    return;
  }
  uint16_t plen = (uint16_t)g_rx[4] | ((uint16_t)g_rx[5] << 8);
  if (plen > IAP_MAX_PAYLOAD) {
    g_sync = 0;
    g_rx_n = 0;
    return;
  }
  uint16_t total = (uint16_t)(IAP_HEADER_LEN + plen + IAP_CRC_LEN);
  if (g_rx_n < total) {
    return;
  }
  uint32_t got;
  memcpy(&got, &g_rx[total - 4], 4);
  uint8_t msg = g_rx[2], seq = g_rx[3];
  g_sync = 0;
  g_rx_n = 0;
  if (got != iap_crc32(g_rx, IAP_HEADER_LEN + plen)) {
    send_nack(seq, IAP_NACK_BAD_CRC);
    return;
  }
  handle_frame(msg, seq, &g_rx[IAP_HEADER_LEN], plen);
}

__attribute__((section(".iap_entry"), used, noinline)) void iap_run(void)
{
  usart_init(HOST_USART);
  g_state = ST_READY;
  g_sync = 0;
  g_rx_n = 0;
  for (;;) {
    uint8_t c;
    if (host_getc(&c)) {
      feed_byte(c);
    }
  }
}

int main(void)
{
  g_state = ST_IDLE;
  g_sync = 0;
  g_rx_n = 0;

#ifdef IAP_USE_USART1
  /* Slave host UART is the live master↔slave link. Do not listen on cold
   * boot: the master app starts talking as soon as it comes up, which can
   * leave this BL stuck in IAP (no SELF_HOLD, wheel freewheels). SoftAP
   * updates enter via iap_run() after the app sees a relayed HELLO. */
  if (app_valid()) {
    jump_to_app();
  }
  usart_init(HOST_USART);
#else
  usart_init(HOST_USART);
  /* Brief listen window, then boot app if valid. */
  for (uint32_t i = 0; i < 300000u; i++) {
    uint8_t c;
    if (host_getc(&c)) {
      feed_byte(c);
      /* Stay in IAP once HELLO moved state. */
      if (g_state != ST_IDLE) {
        break;
      }
    }
  }
  if (g_state == ST_IDLE && app_valid()) {
    jump_to_app();
  }
#endif

  g_state = ST_READY;
  for (;;) {
    uint8_t c;
    if (host_getc(&c)) {
      feed_byte(c);
    }
  }
}
