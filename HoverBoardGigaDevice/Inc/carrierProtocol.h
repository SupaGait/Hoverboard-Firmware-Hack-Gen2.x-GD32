#ifndef CARRIER_PROTOCOL_H
#define CARRIER_PROTOCOL_H

/*
 * CarrierHasArrived board B, the split hoverboard.
 *
 * Same 8-byte command and 18-byte feedback as VARIANT_USART + TANK_STEERING
 * on the single mainboard (hoverboard-firmware-hack-FOC). Little-endian,
 * 115200 8N1, start word 0xABCD.
 *
 *   steer  → left hub  (this firmware: the master half)
 *   speed  → right hub (this firmware: the slave half)
 *
 * A command of ±1000 is ±1000 rpm, the same cap as N_MOT_MAX on the single
 * board. Speed mode holds 0 rpm while frames keep arriving. Silence for
 * CARRIER_LINK_TIMEOUT_MS releases the wheels.
 */

#include <stdint.h>
#include <stddef.h>

#define CARRIER_START_FRAME          0xABCDu
#define CARRIER_START_FRAME_LSB      0xCDu
#define CARRIER_START_FRAME_MSB      0xABu

#define CARRIER_CMD_MAX              1000
#define CARRIER_LINK_TIMEOUT_MS      800u
#define CARRIER_SLAVE_FRESH_MS       200u
#define CARRIER_FEEDBACK_PERIOD_MS   20u

#pragma pack(push, 1)

typedef struct {
	uint16_t start;
	int16_t steer;
	int16_t speed;
	uint16_t checksum;
} CarrierCommand;

typedef struct {
	uint16_t start;
	int16_t cmd1;
	int16_t cmd2;
	int16_t speedR_meas;
	int16_t speedL_meas;
	int16_t batVoltage;
	int16_t boardTemp;
	uint16_t cmdLed;
	uint16_t checksum;
} CarrierFeedback;

#pragma pack(pop)

typedef int CarrierCommandIs8Bytes[(sizeof(CarrierCommand) == 8) ? 1 : -1];
typedef int CarrierFeedbackIs18Bytes[(sizeof(CarrierFeedback) == 18) ? 1 : -1];

static inline uint16_t carrier_command_checksum(uint16_t start, int16_t steer, int16_t speed)
{
	return (uint16_t)(start ^ (uint16_t)steer ^ (uint16_t)speed);
}

static inline uint16_t carrier_feedback_checksum(const CarrierFeedback *fb)
{
	return (uint16_t)(fb->start
		^ (uint16_t)fb->cmd1
		^ (uint16_t)fb->cmd2
		^ (uint16_t)fb->speedR_meas
		^ (uint16_t)fb->speedL_meas
		^ (uint16_t)fb->batVoltage
		^ (uint16_t)fb->boardTemp
		^ fb->cmdLed);
}

/* ±1000 command → revs/s * 1024, which is what DRIVING_MODE 1 consumes.
 * 1000 * 1024 / 60 = 17066, just under 1000 rpm. */
static inline int16_t carrier_cmd_to_speed(int16_t cmd)
{
	return (int16_t)(((int32_t)cmd * 1024) / 60);
}

#endif
