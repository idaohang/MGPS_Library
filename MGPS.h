/***********************************
This is the (modified) Adafruit GPS library - the ultimate GPS library
for the ultimate GPS module!

Tested and works great with the Adafruit Ultimate GPS module
using MTK33x9 chipset
    ------> http://www.adafruit.com/products/746
Pick one up today at the Adafruit electronics shop 
and help support open source hardware & software! -ada

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above must be included in any redistribution
****************************************/

#ifndef _MGPS_H
#define _MGPS_H

#include <SoftwareSerial.h>
#include <HardwareSerial.h>

// how long to wait when we're looking for a response
#define MAXWAITSENTENCE 5

#include "Arduino.h"
#if !defined(__AVR_ATmega32U4__)
 #include "SoftwareSerial.h"
#endif

typedef enum nmeamsg_t {NONE, OTHER, ERROR, GPGGA, GPRMC} nmeamsg_t ;


class MGPS {
 public:


  void begin(uint16_t baud); 

  MGPS(SoftwareSerial *ser); // Constructor when using SoftwareSerial
  MGPS(HardwareSerial *ser); // Constructor when using HardwareSerial

  char *lastNMEA(void);
  boolean newNMEAreceived();
  boolean overrun();
  void common_init(void);
  void sendCommand(char *);
  void pause(boolean b);

  uint8_t parseHex(char c);

  char read(void);
  enum nmeamsg_t parse(char *);
  void interruptReads(boolean r);

  uint8_t hour, minute, seconds, year, month, day;
  uint16_t milliseconds;
  uint32_t fulldate;
  float latitude, longitude, geoidheight, altitude;
  float speed, angle, magvariation, HDOP;
  char lat, lon, mag;
  float latitude_dec, longitude_dec;
  boolean fix;
  uint8_t fixquality, satellites;

  boolean waitForSentence(char *wait, uint8_t max = MAXWAITSENTENCE);
  float distanceTo(float flat2, float flon2);

 private:
  boolean paused;
  
  SoftwareSerial *gpsSwSerial;
  HardwareSerial *gpsHwSerial;
  void updateDecimalLatLon();

};


#endif
