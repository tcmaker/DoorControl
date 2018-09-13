#include "Wiegand2.h"
#include "../PCATTACH/PCATTACH.h"

unsigned long WIEGAND2::_cardTempHigh=0;
unsigned long WIEGAND2::_cardTemp=0;
unsigned long WIEGAND2::_lastWiegand=0;
unsigned long WIEGAND2::_sysTick=0;
unsigned long WIEGAND2::_code=0;
int 		  WIEGAND2::_bitCount=0;	
int			  WIEGAND2::_wiegandType=0;


WIEGAND2::WIEGAND2()
{
}

unsigned long WIEGAND2::getCode()
{
	return _code;
}

int WIEGAND2::getWiegandType()
{
	return _wiegandType;
}

bool WIEGAND2::available()
{
	return DoWiegandConversion();
}

void WIEGAND2::begin()
{
	_lastWiegand = 0;
	_cardTempHigh = 0;
	_cardTemp = 0;
	_code = 0;
	_wiegandType = 0;
	_bitCount = 0;  
	_sysTick=millis();
	//pinMode(D0Pin, INPUT);					// Set D0 pin as input
	//pinMode(D1Pin, INPUT);					// Set D1 pin as input
	//attachInterrupt(0, ReadD0, FALLING);	// Hardware interrupt - high to low pulse
	//attachInterrupt(1, ReadD1, FALLING);	// Hardware interrupt - high to low pulse

}

void WIEGAND2::ReadD0 ()
{
	//Serial.print(0);
	_bitCount++;				// Increment bit count for Interrupt connected to D0
	if (_bitCount>31)			// If bit count more than 31, process high bits
	{
		_cardTempHigh |= ((0x80000000 & _cardTemp)>>31);	//	shift value to high bits
		_cardTempHigh <<= 1;
		_cardTemp <<=1;
		
	}
	else
	{
		_cardTemp <<= 1;		// D0 represent binary 0, so just left shift card data
	}
	_lastWiegand = _sysTick;	// Keep track of last wiegand bit received
}

void WIEGAND2::ReadD1()
{
	//Serial.print(1);
	_bitCount ++;				// Increment bit count for Interrupt connected to D1
	if (_bitCount>31)			// If bit count more than 31, process high bits
	{
		_cardTempHigh |= ((0x80000000 & _cardTemp)>>31);	// shift value to high bits
		_cardTempHigh <<= 1;
		_cardTemp |= 1;
		_cardTemp <<=1;
	}
	else
	{
		_cardTemp |= 1;			// D1 represent binary 1, so OR card data with 1 then
		_cardTemp <<= 1;		// left shift card data
	}
	_lastWiegand = _sysTick;	// Keep track of last wiegand bit received
}

unsigned long WIEGAND2::GetCardId (unsigned long *codehigh, unsigned long *codelow, char bitlength)
{
	unsigned long cardID=0;

	if (bitlength==26)								// EM tag
		cardID = (*codelow & 0x1FFFFFE) >>1;

	if (bitlength==34)								// Mifare 
	{
		*codehigh = *codehigh & 0x03;				// only need the 2 LSB of the codehigh
		*codehigh <<= 30;							// shift 2 LSB to MSB		
		*codelow >>=1;
		cardID = *codehigh | *codelow;
	}
	return cardID;
}

bool WIEGAND2::DoWiegandConversion ()
{
	unsigned long cardID;
	
	
	_sysTick=millis();
	if ((_sysTick - _lastWiegand) > 25)								// if no more signal coming through after 25ms
	{
	
		if ((_bitCount==26) || (_bitCount==34) || (( _bitCount > 0) && (_bitCount < 5))) 	// bitCount for keypress=8, Wiegand 26=26, Wiegand 34=34
		{
			_cardTemp >>= 1;			// shift right 1 bit to get back the real value - interrupt done 1 left shift in advance
			if (_bitCount>32)			// bit count more than 32 bits, shift high bits right to make adjustment
				_cardTempHigh >>= 1;	

			if((_bitCount==26) || (_bitCount==34))		// wiegand 26 or wiegand 34
			{
				cardID = GetCardId (&_cardTempHigh, &_cardTemp, _bitCount);
				_wiegandType=_bitCount;
				_bitCount=0;
				_cardTemp=0;
				_cardTempHigh=0;
				_code=cardID;
				return true;				
			}
			else if (( _bitCount > 0) && (_bitCount < 5))		// keypress wiegand
			{
			    //Serial.println("8 bits");
				
				_code=(int)_cardTemp;				// 0 - 9 keys
				_wiegandType=8;
				_bitCount=0;
				_cardTemp=0;
				
				return true;
				
			}
		}
		else
		{
			// well time over 25 ms and bitCount !=8 , !=26, !=34 , must be noise or nothing then.
			_lastWiegand=_sysTick;
			_bitCount=0;			
			_cardTemp=0;
			_cardTempHigh=0;
			return false;
		}	
	}
	else
		return false;
}