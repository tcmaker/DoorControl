Commands should be CRLF terminated (code checks for LF, a CR in there doesn't seem to have an effect)
All responses are CRLF terminated (\r\n) (per Arduino Serial.println())

door controller command inputs:
	D1U | D2U == unlock door 1 or 2
	D1L | D2L == lock door 1 or 2
	ST == get door status and timestamp
	SDyymmddwhhnnss == set RTC date/time
		yy == 2-digit year (yes, I know...)
		mm == month
		dd == date
		 w == day of week (1-7, starts on Sunday)
		hh == hour (24-hour clock)
		nn == minutes
		ss == seconds
	GT == get RTC time
	DxPy == response from tag query (x=door number, y=tag priority from DB)


door controller responses:
	<ID><command> where:
		ID == [R1 | R2] (keypad ID)
		commands:
			Cn == keypress (n=digits entered, ENT sends; a bare ENT is sent as C0; ESC does not send and clears buffer)
			Tn == tag number (leading zeroes stripped, should match number on tag)
	date string (HH:mm:ss d/m/yy DAY)
	STdd [datestring]
		Status: dd == door 1/2 state (0=unlocked, 1=locked)
