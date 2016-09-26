#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#if defined(ARDUINO_ARCH_SAMD) // ARM Cortex Core Series
	#define SERIALPRINT(x)		{ SerialUSB.print(x); }
	#define SERIALPRINTLN(x)	{ SerialUSB.println(x); }
	#define SERIALFLUSH()		{ SerialUSB.flush(); }
#else
	#define SERIALPRINT(x)		{ Serial.print(x); }
	#define SERIALPRINTLN(x)	{ Serial.println(x); }
	#define SERIALFLUSH()		{ Serial.flush(); }
#endif

#ifdef __AVR__
	#define snprintf		snprintf_P
#else
#endif

#endif //#ifndef _PLATFORM_H_
