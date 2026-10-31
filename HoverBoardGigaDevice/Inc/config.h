#ifndef CONFIG_H
#define CONFIG_H

//#define REMOTE_AUTODETECT
				// ONLY test with 1-2A constant current power supply !!!! The charger with 1.5A might also do :-)
				// will drive the motor without hall input to detect the hall pins..

#ifdef REMOTE_AUTODETECT
	#define WINDOWS_RN		// adds a \r before every \n
	
	//#define RTT_REMOTE

	#define REMOTE_USART				0 	// 	1 is usually PA2/PA3 and the original master-slave 4pin header
																	//	0 is usually PB6/PB7 and the empty header close to the flash-header
																	//	2 is usually PB10/PB11 on stm32f103 boards
																	
#else

	// choose your target in the dropdown list to the top-right of the Keil IDE
	// and then set your layout below
	// Gen2-target-layout is included in defines.h
	#ifdef GD32F130		// TARGET = 1
		#define LAYOUT 1
		#define LAYOUT_SUB 1	// Layout 2.1.7 exisits as 2.1.7.0 and 2.1.7.1
	#elif GD32F103		// TARGET = 2
		#define LAYOUT 1
		//#define STM32F103 	// uncomment if your board has stm32f103 instead of gd32f103. Will give the max 64 Mhz for STM32
			// Warning: will overclock gd32f103 mcu to 124 MHz and doubling current consumption. Keep GD32 to its 72 Mhz.
	#elif GD32E230		// TARGET = 3
		#define LAYOUT 1
	#elif MM32SPIN05	// TARGET = 4
		#define LAYOUT 1
	#endif

	//#define BLDC_BC			// old block commutation bldc control
	#define BLDC_SINE			// silent sine-pwm motor control, added 2025 by Robo Durden. 
	//#define BLDC_SINE_BOOSTER		// can boost speed by 15% starting from 87% throttle.
	
	// CarrierHasArrived board B. Comment this out to return to the stock single-board demo.
	#define CARRIER

	#ifdef CARRIER
		// Speed mode: a 0 command holds 0 rpm. ±1000 from the ESP32 is ±1000 rpm.
		#define DRIVING_MODE 1	//  0=pwm, 1=speed in revs/s*1024, 2=torque in NewtonMeter*1024, 3=iOdometer
		#define SPEED_AsRevsPerSec
		// No reverse beep: the base reverses often. No idle latch-off: it sits armed.
	#else
		#define DRIVING_MODE 0	//  0=pwm, 1=speed in revs/s*1024, 2=torque in NewtonMeter*1024, 3=iOdometer
		//#define SPEED_AsRevsPerSec		// Will overflow at 327 revs/s = 19620 rpm. Hoverboard motor: 14 rpm/V * 50V = 700 rpm
		#define BEEP_BACKWARDS
	#endif

	#define BAT_CELLS         	10        // battery number of cells. Normal Hoverboard battery: 10s
	//#define BATTERY_LOW_SHUTOFF		// will shut off the board below BAT_LOW_DEAD = BAT_CELLS * CELL_LOW_DEAD, 
	#define BATTERY_LOW_BEEP		// will start beeping for different battery low lwevels

	//#define DEBUG_LED		// uncomment to activate DEBUG_LedSet(bSet,iColor) macro. iCol: 0=green, 1=organge, 2=red


	// Carrier: default image is the master half (ESP32 UART + the other wheel).
	// Slave image: `make carrier-slave` or `pio run -e carrier_slave_GD32F130`
	// which adds -DCARRIER_SLAVE. Flash one of each.
	#ifdef CARRIER_SLAVE
		#define SLAVE
	#elif defined(CARRIER)
		#define MASTER
	#else
		//#define MASTER		// uncomment for MASTER firmware.
		//#define SLAVE			// uncomment for SLAVE firmware.
		#define SINGLE			// uncomment if firmware is for single board and no master-slave dual board setup
	#endif

	#if defined(MASTER) || defined(SINGLE)
		
		// choose only one 'remote' to control the motor
		#ifdef CARRIER
			#define REMOTE_CARRIER	// 8-byte / 18-byte VARIANT_USART frame, tank steering
		#else
			#define REMOTE_DUMMY
		#endif
				#ifdef REMOTE_DUMMY
					#define REMOTE_PERIOD 6 // 3 = 3 seconds period of the zigzag curve
					#define TEST_HALL2LED	// led the 3-led panel blink according to the hall sensors
					#define RTT_REMOTE	// log and set via ST-Link dongle
						#define WINDOWS_RN		// adds a \r before every \n to RTT log
				#endif
		//#define REMOTE_UART
		//#define REMOTE_UARTBUS	// ESP32 as master and multiple boards as multiple slaves ESP.tx-Hovers.rx and ESP.rx-Hovers.tx
				#ifdef REMOTE_UARTBUS
					#define SLAVE_ID	0		// must be unique for all hoverboards connected to the bus
				#endif
		//#define REMOTE_CRSF		// https://github.com/RoboDurden/Hoverboard-Firmware-Hack-Gen2.x/issues/26
		//#define REMOTE_ROS2		// https://github.com/RoboDurden/Hoverboard-Firmware-Hack-Gen2.x/issues/122
		//#define REMOTE_ADC	// speed is PA2=TX and steer is PA3=RX of the masterslave header. Get 3.3V from the flash header
												// DO NOT use the 5V/15V pin of the masterslave header for the potentiometers !!!!!!!!!
												// SLAVE board has to be connected to the additional UART header, but 5V/15V coming from the masters masterslave header
												// for calibration, hold the on/off button until the startup melody restarts.
												// Then release the button and leave the joystick (the two potentiometers) in neutral position.
												// When the melody returns for 2 seconds, push speed to max.
												// After another 5 seconds + 2 seconds melody: push speed to min. Then steer to max. Finally steer to min
		//#define REMOTE_OPTIMIZEPID		// will zigzag motor and optimize pid parameters for 1=speed or 3=iOdometer.
		// monitor with StmStudio/McuViewer or via ST-Link usb dongle RTT and openocd_rtt_32f1x.bat or PlatformIO RTT_Task
				#ifdef REMOTE_OPTIMIZEPID
					#define WINDOWS_RN		// adds a \r before every \n		
				#endif


		//#define TEST_HALL2LED	// led the 3-led panel blink according to the hall sensors

		//#define RTT_REMOTE		// uncoment for i2c_scanner and dump_i2c_registers to print to rtt :-)
			// needs either I2C_PB8PB9 or I2C_PB6PB7 and MPU_6050 or MPU_6050old or MPU_6500 or BMI_160 in your defines_2-x-y.h file
		//#define SEND_IMU_DATA // send the IMU data with RemoteUart or RemoteUartBus. Tested for 2.1.20 !

		//#define PILOT_USER	// uncomment if you want to extend the firmware with custom code :-)
		//#define PILOT_HOVERBIKE	// very experimental pedal detection with chatGpt5 :-/
		#define SPEED_COEFFICIENT   -1	// only used if no PILOT_XY is defined
		#define STEER_COEFFICIENT   1		// only used if no PILOT_XY is defined
		
		//#define DISABLE_BUTTON	// this is the opposite of former CHECK_BUTTON define.
															// remove '//' if you use a slave board as master 
															// or if you turn the boards on/off by injecting a postive voltage into the input pin of the 2pin BUTTON header

		//#define DISABLE_CHARGESTATE	//active this if you test with the charger plugged in as the power supply with max 1.5A

		#define MASTER_OR_SINGLE
	#endif
	
	#if defined(MASTER) || defined(SLAVE)
		#define MASTERSLAVE_USART		1 	// 	1 is usually PA2/PA3 and the original master-slave 4pin header
																		//	0 is usually PB6/PB7 and the empty header close to the flash-header
																		//	2 is usually PB10/PB11 on stm32f103 boards
	#endif
	
	#if defined(REMOTE_UART) || defined(REMOTE_UARTBUS) || defined(REMOTE_CRSF) || defined(REMOTE_ROS2) || defined(REMOTE_CARRIER)
		#define REMOTE_USART				0 	// 	1 is usually PA2/PA3 and the original master-slave 4pin header
																		//	0 is usually PB6/PB7 and the empty header close to the flash-header
																		//	2 is usually PB10/PB11 on stm32f103 boards
																		// Carrier: ESP32 UART2 (GPIO15 TX, GPIO16 RX) lands on this header.
																		// Leave USART1 on the 4-pin cable between the two halves.
	#endif
	
	
#endif



#ifdef MASTER_OR_SINGLE
	#ifndef CARRIER
		#define INACTIVITY_TIMEOUT 	8        	// Minutes of not driving until poweroff (not very precise)
	#endif

	#define CELL_LOW_LVL1     3.5       // Gently beeps, show green battery symbol above this Level.
	#define CELL_LOW_LVL2     3.3       // Battery almost empty, show orange battery symbol above this Level. Charge now! 
	#define CELL_LOW_DEAD     3.0       // Undervoltage lockout, show red battery symbol above this Level.
#endif


// Motor DC current limit in amps (BLDC ISR PWM chop in bldc.c).
// Carrier board B: lower than FOC I_MOT_MAX (15 A) so a speed-mode stall does not
// hold ~15 A chop long enough to disrupt the master↔slave USART (EMI / bus sag),
// which otherwise expires CarrierSlaveFresh and silences ESP feedback → SoftAP B-down.
// Non-Carrier builds keep the stock 15 A.
#ifdef CARRIER
	#define DC_CUR_LIMIT     		8
#else
	#define DC_CUR_LIMIT     		15
#endif

#endif		// CONFIG_H
