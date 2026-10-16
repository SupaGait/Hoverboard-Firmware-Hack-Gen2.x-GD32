#ifndef DEFINES_2_TARGET_VERSION_H
#define DEFINES_2_TARGET_VERSION_H

/*
 * Selected by defines.h. config.h sets TARGET (via the chip) and LAYOUT.
 * File name is defines_2-<TARGET>-<LAYOUT>.h, the same numbering as the
 * board silkscreen 2.<target>.<layout>. Default carrier image is GD32F130
 * layout 1 (defines_2-1-1.h): USART0 PB6/PB7 to the ESP32, USART1 PA2/PA3
 * between the halves.
 */

#ifndef LAYOUT
	#error "Set LAYOUT in config.h"
#endif

#if TARGET == 1
	#if LAYOUT == 1
		#include "defines_2-1-1.h"
	#elif LAYOUT == 2
		#include "defines_2-1-2.h"
	#elif LAYOUT == 3
		#include "defines_2-1-3.h"
	#elif LAYOUT == 4
		#include "defines_2-1-4.h"
	#elif LAYOUT == 5
		#include "defines_2-1-5.h"
	#elif LAYOUT == 6
		#include "defines_2-1-6.h"
	#elif LAYOUT == 7
		#include "defines_2-1-7.h"
	#elif LAYOUT == 8
		#include "defines_2-1-8.h"
	#elif LAYOUT == 9
		#include "defines_2-1-9.h"
	#elif LAYOUT == 10
		#include "defines_2-1-10.h"
	#elif LAYOUT == 11
		#include "defines_2-1-11.h"
	#elif LAYOUT == 12
		#include "defines_2-1-12.h"
	#elif LAYOUT == 13
		#include "defines_2-1-13.h"
	#elif LAYOUT == 14
		#include "defines_2-1-14.h"
	#elif LAYOUT == 15
		#include "defines_2-1-15.h"
	#elif LAYOUT == 16
		#include "defines_2-1-16.h"
	#elif LAYOUT == 18
		#include "defines_2-1-18.h"
	#elif LAYOUT == 20
		#include "defines_2-1-20.h"
	#elif LAYOUT == 22
		#include "defines_2-1-22.h"
	#elif LAYOUT == 28
		#include "defines_2-1-28.h"
	#else
		#error "No defines_2-1-<LAYOUT>.h for this LAYOUT. Set LAYOUT in config.h."
	#endif
#elif TARGET == 2
	#if LAYOUT == 1
		#include "defines_2-2-1.h"
	#elif LAYOUT == 3
		#include "defines_2-2-3.h"
	#elif LAYOUT == 6
		#include "defines_2-2-6.h"
	#elif LAYOUT == 7
		#include "defines_2-2-7.h"
	#elif LAYOUT == 8
		#include "defines_2-2-8.h"
	#elif LAYOUT == 20
		#include "defines_2-2-20.h"
	#else
		#error "No defines_2-2-<LAYOUT>.h for this LAYOUT. Set LAYOUT in config.h."
	#endif
#elif TARGET == 3
	#if LAYOUT == 1
		#include "defines_2-3-1.h"
	#elif LAYOUT == 2
		#include "defines_2-3-2.h"
	#elif LAYOUT == 3
		#include "defines_2-3-3.h"
	#else
		#error "No defines_2-3-<LAYOUT>.h for this LAYOUT. Set LAYOUT in config.h."
	#endif
#elif TARGET == 4
	#if LAYOUT == 1
		#include "defines_2-4-1.h"
	#else
		#error "No defines_2-4-<LAYOUT>.h for this LAYOUT. Set LAYOUT in config.h."
	#endif
#else
	#error "Unknown TARGET. Use 1=GD32F130, 2=GD32F103, 3=GD32E230, 4=MM32."
#endif

#endif
