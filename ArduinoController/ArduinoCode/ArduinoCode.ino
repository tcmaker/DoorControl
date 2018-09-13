#include <Wire.h>         // Needed for I2C Connection to the DS1307 date/time chip
#include <EEPROM.h>       // Needed for saving to non-voilatile memory on the Arduino.
#include <avr/pgmspace.h> // Allows data to be stored in FLASH instead of RAM
#define MCU328         // Set this if using any boards other than the "Mega"
#define HWV3STD        // Use this option if using Open access v3 Standard board

#define MCPIOXP        // this if using the v3 hardware with the MCP23017 i2c IO chip
#define AT24EEPROM     // Set this if you have the At24C i2c EEPROM chip installed



#include <DS1307.h>             // DS1307 RTC Clock/Date/Time chip library
#include <PCATTACH.h> 
PCATTACH pcattach;
#include <Wiegand.h>
#include <Wiegand2.h>
#include <Timer.h>

#ifdef MCPIOXP
#include <Adafruit_MCP23017.h>  // Library for the MCP23017 i2c I/O expander
#endif

#ifdef HWV3STD                          // Use these pinouts for the v3 Standard hardware
#define DOORPIN1       6                // Define the pin for electrified door 1 hardware. (MCP)
#define DOORPIN2       7                // Define the pin for electrified door 2 hardware  (MCP)
#define ALARMSTROBEPIN 8                // Define the "non alarm: output pin. Can go to a strobe, small chime, etc. Uses GPB0 (MCP pin 8).
#define ALARMSIRENPIN  9                // Define the alarm siren pin. This should be a LOUD siren for alarm purposes. Uses GPB1 (MCP pin9).
#define READER1GRN     10
#define READER1BUZ     11
#define READER2GRN     12
#define READER2BUZ     13
#define RS485ENA        6               // Arduino Pin D6
#define STATUSLED       14              // MCP pin 14
#define R1ZERO          2
#define R1ONE           3
#define R2ZERO          4
#define R2ONE           5
#endif


/*  Global Boolean values
 *
 */
const long overridecommand = 0x124513; 
String inputString = "";         // a string to hold incoming serial data

bool     door0Locked=true;                          // Keeps track of whether the doors are supposed to be locked right now
bool     door1Locked=true;
boolean  doorChime=false;                        // Keep track of when door chime last activated
boolean  doorClosed=false;                       // Keep track of when door last closed for exit delay
boolean  sensor[4]={
  false};                      // Keep track of tripped sensors, do not log again until reset.
unsigned long keycommand = 0;
#ifdef MCPIOXP
Adafruit_MCP23017 mcp;
#endif
WIEGAND wg;
WIEGAND2 wg2;
unsigned long door0RelockTime = 0;
unsigned long door1RelockTime = 0;
unsigned long currentTime = 0;
DS1307 ds1307;        // RTC Instance
void setup()
{
  Wire.begin();   // start Wire library as I2C-Bus Master
  mcp.begin();      // use default address 0
  currentTime = millis();
  
  pinMode(2,INPUT);                // Initialize the Arduino built-in pins
  pinMode(3,INPUT);
  pinMode(4,INPUT);
  pinMode(5,INPUT);
  mcp.pinMode(DOORPIN1, OUTPUT); 
  mcp.pinMode(DOORPIN2, OUTPUT);
  pinMode(6,OUTPUT);
  pcattach.PCattachInterrupt(R1ZERO, wg.ReadD0, FALLING); 
  pcattach.PCattachInterrupt(R1ONE, wg.ReadD1, FALLING);  
  pcattach.PCattachInterrupt(R2ZERO, wg2.ReadD0, FALLING); 
  pcattach.PCattachInterrupt(R2ONE, wg2.ReadD1, FALLING);  
  pinMode(READER1GRN, OUTPUT);
  
  for(int i=0; i<=15; i++)        // Initialize the I/O expander pins
  {
    mcp.pinMode(i, OUTPUT);
  }

  Serial.begin(9600);  
  wg.begin();
}
void loop()
{
  checkReader0();
  checkReader1();
  checkDoorRelock();
}

void checkReader0() {
  unsigned long code=0;
  if(wg.available())
  {
    code = wg.getCode();
    int readtype = wg.getWiegandType();

    if (readtype==8 && code < 0xa)
    {
      keycommand = keycommand << 4;
      keycommand |= code;
    }
    else if (readtype==8 && code == 0xb)
    {
      Serial.print("R1C");
      Serial.println(keycommand,HEX);
      if (keycommand == overridecommand)
      {
        doorUnlock(1);
      }
      if (keycommand == 0x5)
      {
        doorLock(1); 
      }
      keycommand = 0;
    }
    else if (readtype==8 && code == 0xa)
    {
      //Serial.println("R1CC");
      keycommand = 0;
    }
    if (readtype==26)
    {
      Serial.print("R1T");
      Serial.println(code,DEC);
    }
  }
}
void checkReader1() {
  unsigned long code=0;
  if(wg2.available())
  {
    code = wg2.getCode();
    int readtype = wg2.getWiegandType();

    if (readtype==8 && code < 0xa)
    {
      keycommand = keycommand << 4;
      keycommand |= code;
    }
    else if (readtype==8 && code == 0xb)
    {
      Serial.print("R2C");
      Serial.println(keycommand,HEX);
      if (keycommand == overridecommand)
      {
        doorUnlock(2);
      }
      if (keycommand == 0x5)
      {
        doorLock(2);
      }
      keycommand = 0;
    }
    else if (readtype==8 && code == 0xa)
    {
      //Serial.println("R2CC");
      keycommand = 0;
    }
    if (readtype==26)
    {
      Serial.print("R2T");
      Serial.println(code,DEC);
    }

  }
}
//Gets Called when there is serial input
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      execSerialCmd();
      //Serial.println(inputString);
    } 
  }
}
//Execute Commands From Serial Input
void execSerialCmd()
{
  if (inputString.startsWith("D1U"))
  {
    doorUnlock(0);
  }
  if (inputString.startsWith("D2U"))
  {
    doorUnlock(1);
  }
  if (inputString.startsWith("D1L"))
  {
    doorLock(0);
  }
  if (inputString.startsWith("D2L"))
  {
    doorLock(1);
  }
  if (inputString.startsWith("ST"))
  {
    //setup call to set rtc
    //parse inputString to get the following
    int year = inputString.substring(2,4).toInt();
    int month = inputString.substring(4,6).toInt();
    int day = inputString.substring(6,8).toInt();
    int dayOfWeek = inputString.substring(8,9).toInt();
    int hour = inputString.substring(9,11).toInt();
    int minute = inputString.substring(11,13).toInt();
    int second = inputString.substring(13,15).toInt();
    Serial.print("Year:");
    Serial.println(year);
    Serial.print("Month:");
    Serial.println(month);
    Serial.print("Day:");
    Serial.println(day);
    Serial.print("DayOfWeek:");
    Serial.println(dayOfWeek);
    Serial.print("hour:");
    Serial.println(hour);
    Serial.print("Min:");
    Serial.println(minute);
    Serial.print("Second:");
    Serial.println(second);
    
    setTime(year, month, day, dayOfWeek, hour, minute, second);
  }
  if (inputString.startsWith("GT"))
  {
    logDate();
  }  
  inputString=""; 
}

void checkDoorRelock() {
  
  if (!door0Locked)
  {
    if (door0RelockTime < millis())
    {
      doorLock(0);
    }
    if (door1RelockTime < millis())
    {
      doorLock(0);
    }
  }
}


// Misc Functions

void doorUnlock(int input) {          //Send an unlock signal to the door and flash the Door LED

  uint8_t dp=1;
  uint8_t dpLED=1;

  if(input == 1) 
  {
    dp=DOORPIN1; 
    dpLED=READER1GRN;
  }
  else 
  {
    dp=DOORPIN2;
    dpLED=READER2GRN;
  }
  mcp.digitalWrite(dp, HIGH);
  mcp.digitalWrite(dpLED, HIGH); 
}

void doorLock(int input) {          //Send an unlock signal to the door and flash the Door LED
  uint8_t dp=1;
  uint8_t dpLED=1;
  if(input == 1) 
  {
    dp=DOORPIN1; 
    dpLED=READER1GRN;
  }
  else 
  {
    dp=DOORPIN2;
    dpLED=READER2GRN;
  }
  mcp.digitalWrite(dp, LOW);
  mcp.digitalWrite(dpLED, LOW);
}

// Change date/time 
void setTime(uint8_t year, uint8_t month, uint8_t day, uint8_t dayOfWeek , uint8_t hour, uint8_t minute, uint8_t second) {  // Change date/time 
  // ParseTime from input char array
  Serial.print("Old time :");           
  logDate();
  Serial.println();
  ds1307.setDateDs1307(second, minute, hour, dayOfWeek, day, month, year);
  Serial.print("New time :");
  logDate();
  Serial.println();
 }

void logDate()
{
  uint8_t second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  ds1307.getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  Serial.print(hour, DEC);
  Serial.print(":");
  Serial.print(minute, DEC);
  Serial.print(":");
  Serial.print(second, DEC);
  Serial.print("  ");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  Serial.print(' ');
  
  switch(dayOfWeek){

    case 1:{
     Serial.print("SUN");
     break;
           }
    case 2:{
     Serial.print("MON");
     break;
           }
    case 3:{
     Serial.print("TUE");
     break;
          }
    case 4:{
     Serial.print("WED");
     break;
           }
    case 5:{
     Serial.print("THU");
     break;
           }
    case 6:{
     Serial.print("FRI");
     break;
           }
    case 7:{
     Serial.print("SAT");
     break;
           }  
  }
  
  Serial.print(" ");

}


