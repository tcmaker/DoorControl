#ifndef _WIEGAND_H
#define _WIEGAND_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


#define D0Pin 2		// Arduino Pin 2 Hardware interrupt
#define D1Pin 3  	// Arduino Pin 3 Hardware interrupt


class WIEGAND {

public:
	WIEGAND();
	void begin();
	bool available();
	unsigned long getCode();
	int getWiegandType();
	static void ReadD0();
	static void ReadD1();
private:
	
	static bool DoWiegandConversion ();
	static unsigned long GetCardId (unsigned long *codehigh, unsigned long *codelow, char bitlength);
	
	static unsigned long 	_cardTempHigh;
	static unsigned long 	_cardTemp;
	static unsigned long 	_lastWiegand;
	static unsigned long 	_sysTick;
	static int				_bitCount;	
	static int				_wiegandType;
	static unsigned long	_code;
	
/*  static 
	void ReadD0();
	void ReadD1();
	bool DoWiegandConversion ();
	unsigned long GetCardId (unsigned long *codehigh, unsigned long *codelow, char bitlength);
	
	unsigned long 	_cardTempHigh;
	unsigned long 	_cardTemp;
	unsigned long 	_lastWiegand;
	unsigned long 	_sysTick;
	int				_bitCount;	
	int				_wiegandType;
	unsigned long	_code;

*/
};

#endif
