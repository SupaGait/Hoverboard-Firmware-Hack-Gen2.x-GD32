#ifndef REMOTE_CARRIER_H
#define REMOTE_CARRIER_H

#include "carrierProtocol.h"

#define REMOTE_BAUD 115200

#define SEND_INTERVAL_MS CARRIER_FEEDBACK_PERIOD_MS
#define LOST_CONNECTION_STOP_MILLIS CARRIER_LINK_TIMEOUT_MS

/* Tank assignment lives in Pilot(); the stock steer/speed mixer must not run. */
#undef PILOT_DEFAULT
void Pilot(int16_t *pPwmMaster, int16_t *pPwmSlave);

#endif
