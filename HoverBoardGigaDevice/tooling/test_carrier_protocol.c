/* Host check of the CarrierHasArrived board-B frame. No MCU headers. */
#include "carrierProtocol.h"

#include <stdio.h>
#include <string.h>

static int fails;

static void expect_eq(const char *name, int got, int want)
{
	if (got != want) {
		printf("FAIL %s: got %d want %d\n", name, got, want);
		fails++;
	}
}

int main(void)
{
	const uint8_t command_bytes[8] = {0xCD, 0xAB, 0x02, 0x01, 0xFE, 0xFF, 0x31, 0x55};
	uint8_t raw[8];
	CarrierCommand cmd;
	CarrierFeedback fb;
	uint16_t fb_sum;

	expect_eq("command length", (int)sizeof(CarrierCommand), 8);
	expect_eq("feedback length", (int)sizeof(CarrierFeedback), 18);
	expect_eq("steer offset", (int)offsetof(CarrierCommand, steer), 2);
	expect_eq("speed offset", (int)offsetof(CarrierCommand, speed), 4);
	expect_eq("command checksum offset", (int)offsetof(CarrierCommand, checksum), 6);
	expect_eq("cmd1 offset", (int)offsetof(CarrierFeedback, cmd1), 2);
	expect_eq("cmd2 offset", (int)offsetof(CarrierFeedback, cmd2), 4);
	expect_eq("speedR offset", (int)offsetof(CarrierFeedback, speedR_meas), 6);
	expect_eq("speedL offset", (int)offsetof(CarrierFeedback, speedL_meas), 8);
	expect_eq("voltage offset", (int)offsetof(CarrierFeedback, batVoltage), 10);
	expect_eq("temp offset", (int)offsetof(CarrierFeedback, boardTemp), 12);
	expect_eq("led offset", (int)offsetof(CarrierFeedback, cmdLed), 14);
	expect_eq("feedback checksum offset", (int)offsetof(CarrierFeedback, checksum), 16);

	/* 1000 on the wire is 1000 rpm, stored as revs/s * 1024. */
	expect_eq("1000 rpm scale", carrier_cmd_to_speed(1000), 17066);
	expect_eq("reverse scale", carrier_cmd_to_speed(-1000), -17066);
	expect_eq("zero scale", carrier_cmd_to_speed(0), 0);
	/* main.c negates the right-wheel command before the slave converts it. */
	expect_eq("right hub setpoint", carrier_cmd_to_speed((int16_t)(-1000)), -17066);

	cmd.start = CARRIER_START_FRAME;
	cmd.steer = 0x0102; /* left hub */
	cmd.speed = -2;     /* right hub */
	cmd.checksum = carrier_command_checksum(cmd.start, cmd.steer, cmd.speed);
	expect_eq("command checksum", cmd.checksum, 0x5531);
	memcpy(raw, &cmd, sizeof(raw));
	if (memcmp(raw, command_bytes, sizeof(raw)) != 0) {
		printf("FAIL command bytes\n");
		fails++;
	}

	memset(&fb, 0, sizeof(fb));
	fb.start = CARRIER_START_FRAME;
	fb.cmd1 = 10;
	fb.cmd2 = -20;
	fb.speedR_meas = -30; /* right motor rpm, negative when that command is positive */
	fb.speedL_meas = 40;
	fb.batVoltage = 3600;
	fb.boardTemp = 0;
	fb.cmdLed = 0;
	fb.checksum = carrier_feedback_checksum(&fb);
	fb_sum = (uint16_t)(CARRIER_START_FRAME ^ (uint16_t)10 ^ (uint16_t)(int16_t)(-20)
		^ (uint16_t)(int16_t)(-30) ^ (uint16_t)40 ^ (uint16_t)3600);
	expect_eq("feedback checksum", fb.checksum, fb_sum);
	expect_eq("link timeout ms", (int)CARRIER_LINK_TIMEOUT_MS, 800);

	if (fails) {
		printf("%d failed\n", fails);
		return 1;
	}
	printf("ok\n");
	return 0;
}
