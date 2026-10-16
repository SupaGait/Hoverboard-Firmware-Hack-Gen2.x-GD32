#include "../Inc/defines.h"
#include "../Inc/it.h"
#include "../Inc/comms.h"
#include "../Inc/commsMasterSlave.h"
#include "../Inc/bldc.h"
#include <string.h>

#ifdef REMOTE_CARRIER

#if DRIVING_MODE != 1
	#error "Carrier board B requires DRIVING_MODE 1 (speed, 0 holds 0 rpm)"
#endif
#ifndef SPEED_AsRevsPerSec
	#error "Carrier board B requires SPEED_AsRevsPerSec so feedback is rpm"
#endif
#ifndef MASTER
	#error "REMOTE_CARRIER belongs on the master half. Build the slave with CARRIER_SLAVE."
#endif

extern uint8_t usart0_rx_buf[1];
extern uint8_t usart1_rx_buf[1];
extern uint8_t usart2_rx_buf[1];
extern uint8_t bRemoteTimeout;
extern int32_t steer;
extern int32_t speed;
extern DataSlave oDataSlave;
extern float batteryVoltage;
extern float realSpeed;

static int16_t iReceivePos = -1;
static uint8_t aReceiveBuffer[sizeof(CarrierCommand)];
static volatile uint32_t iTimeLastRx = 0;
static volatile uint8_t bRxSeen = 0;
static uint32_t iTimeNextTx = 0;

static int16_t clamp_cmd(int32_t cmd)
{
	if (cmd > CARRIER_CMD_MAX)
		return CARRIER_CMD_MAX;
	if (cmd < -CARRIER_CMD_MAX)
		return -CARRIER_CMD_MAX;
	return (int16_t)cmd;
}

/* steer is the left hub (master). speed is the right hub (slave).
 * main.c sends the slave the negated right command, which is FOC's
 * pwmr = -cmdR while INVERT_R_DIRECTION is unset. */
void Pilot(int16_t *pPwmMaster, int16_t *pPwmSlave)
{
	int16_t left = clamp_cmd(steer);
	int16_t right = clamp_cmd(speed);

	*pPwmMaster = carrier_cmd_to_speed(left);
	*pPwmSlave = right;
}

void RemoteUpdate(void)
{
	uint8_t linkLost = !bRxSeen
		|| ((millis() - iTimeLastRx) > CARRIER_LINK_TIMEOUT_MS);

	if (linkLost)
	{
		bRemoteTimeout = 1;
		speed = 0;
		steer = 0;
	}

	/* No feedback until the slave half has reported in. The ESP32 then
	 * keeps this axle (and the other board) at zero instead of driving
	 * one hub. The master wheel itself holds 0 while ESP32 frames continue. */
	if (!CarrierSlaveFresh())
	{
		speed = 0;
		steer = 0;
		return;
	}

	if (millis() < iTimeNextTx)
		return;
	iTimeNextTx = millis() + SEND_INTERVAL_MS;

	{
		CarrierFeedback fb;
		int16_t left = clamp_cmd(steer);
		int16_t right = clamp_cmd(speed);

		fb.start = CARRIER_START_FRAME;
		fb.cmd1 = left;
		fb.cmd2 = right;
		/* Slave realSpeed already carries the negated command's sign. */
		fb.speedR_meas = (int16_t)(oDataSlave.realSpeed * 60.0f);
		fb.speedL_meas = (int16_t)(realSpeed * 60.0f);
		fb.batVoltage = (int16_t)(batteryVoltage * 100.0f);
		fb.boardTemp = 0; /* this tree does not sample the die sensor */
		fb.cmdLed = 0;
		fb.checksum = carrier_feedback_checksum(&fb);
		SendBuffer(USART_REMOTE, (uint8_t *)&fb, sizeof(fb));
	}
}

void RemoteCallback(void)
{
	uint8_t cRead = USART_REMOTE_BUFFER[0];

	if (iReceivePos < 0)
	{
		if (cRead == CARRIER_START_FRAME_LSB)
			iReceivePos = 0;
	}
	else if (iReceivePos == 1)
	{
		if (cRead != CARRIER_START_FRAME_MSB)
			iReceivePos = -1;
	}

	if (iReceivePos >= 0)
	{
		aReceiveBuffer[iReceivePos++] = cRead;
		if (iReceivePos == (int16_t)sizeof(CarrierCommand))
		{
			CarrierCommand cmd;
			memcpy(&cmd, aReceiveBuffer, sizeof(cmd));
			if (cmd.start == CARRIER_START_FRAME
				&& cmd.checksum == carrier_command_checksum(cmd.start, cmd.steer, cmd.speed))
			{
				iTimeLastRx = millis();
				bRxSeen = 1;
				bRemoteTimeout = 0;
				steer = cmd.steer;
				speed = cmd.speed;
				ResetTimeout();
			}
			iReceivePos = -1;
		}
	}
}

#endif
