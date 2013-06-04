/***********************************
This is our GPS library

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, check license.txt for more information
All text above must be included in any redistribution
****************************************/

#include <MGPS.h>

// how long are max NMEA lines to parse?
#define MAXLINELENGTH 120

// we double buffer: read one line in and leave one for the main program
volatile char line1[MAXLINELENGTH];
volatile char line2[MAXLINELENGTH];
// our index into filling the current line
volatile uint8_t lineidx=0;
// pointers to the double buffers
volatile char *currentline;
volatile char *lastline;
volatile boolean recvdflag;
volatile boolean overrunflag;
volatile boolean inStandbyMode;


enum nmeamsg_t MGPS::parse(char *nmea) {
  // do checksum check

  // first look if we even have one
  if (nmea[strlen(nmea)-4] == '*') {
    uint16_t sum = parseHex(nmea[strlen(nmea)-3]) * 16;
    sum += parseHex(nmea[strlen(nmea)-2]);
    
    // check checksum 
    for (uint8_t i=1; i < (strlen(nmea)-4); i++) {
      sum ^= nmea[i];
    }
    if (sum != 0) {
      // bad checksum :(
      //return false;
    }
  }

  // look for a few common sentences
  if (strstr(nmea, "$GPGGA")) {
    // found GGA
    char *p = nmea;
    // get time
    p = strchr(p, ',')+1;
    float timef = atof(p);
    uint32_t time = timef;
    hour = time / 10000;
    minute = (time % 10000) / 100;
    seconds = (time % 100);

    milliseconds = fmod(timef, 1.0) * 1000;

    // parse out latitude
    p = strchr(p, ',')+1;
    latitude = atof(p);

    p = strchr(p, ',')+1;
    if (p[0] == 'N') lat = 'N';
    else if (p[0] == 'S') lat = 'S';
    else if (p[0] == ',') lat = 0;
    else return ERROR;

    // parse out longitude
    p = strchr(p, ',')+1;
    longitude = atof(p);

    p = strchr(p, ',')+1;
    if (p[0] == 'W') lon = 'W';
    else if (p[0] == 'E') lon = 'E';
    else if (p[0] == ',') lon = 0;
    else return ERROR;
	updateDecimalLatLon();

    p = strchr(p, ',')+1;
    fixquality = atoi(p);

    p = strchr(p, ',')+1;
    satellites = atoi(p);

    p = strchr(p, ',')+1;
    HDOP = atof(p);

    p = strchr(p, ',')+1;
    altitude = atof(p);
    p = strchr(p, ',')+1;
    p = strchr(p, ',')+1;
    geoidheight = atof(p);
    return GPGGA;
  }
  if (strstr(nmea, "$GPRMC")) {
   // found RMC
    char *p = nmea;

    // get time
    p = strchr(p, ',')+1;
    float timef = atof(p);
    uint32_t time = timef;
    hour = time / 10000;
    minute = (time % 10000) / 100;
    seconds = (time % 100);

    milliseconds = fmod(timef, 1.0) * 1000;

    p = strchr(p, ',')+1;
    // Serial.println(p);
    if (p[0] == 'A') 
      fix = true;
    else if (p[0] == 'V')
      fix = false;
    else
      return ERROR;

    // parse out latitude
    p = strchr(p, ',')+1;
    latitude = atof(p);

    p = strchr(p, ',')+1;
    if (p[0] == 'N') lat = 'N';
    else if (p[0] == 'S') lat = 'S';
    else if (p[0] == ',') lat = 0;
    else return ERROR;

    // parse out longitude
    p = strchr(p, ',')+1;
    longitude = atof(p);

    p = strchr(p, ',')+1;
    if (p[0] == 'W') lon = 'W';
    else if (p[0] == 'E') lon = 'E';
    else if (p[0] == ',') lon = 0;
    else return ERROR;
	updateDecimalLatLon();

    // speed
    p = strchr(p, ',')+1;
    speed = atof(p);

    // angle
    p = strchr(p, ',')+1;
    angle = atof(p);

    p = strchr(p, ',')+1;
    fulldate = atof(p);
    day = fulldate / 10000;
    month = (fulldate % 10000) / 100;
    year = (fulldate % 100);

    // we dont parse the remaining, yet!
    return GPRMC;
  }
  if (strstr(nmea, "$GP"))
    return OTHER;
  else
    return NONE;
}

void MGPS::updateDecimalLatLon() {
    // calculate signed degree-decimal value of latitude term
    latitude_dec = latitude / 100.0;
    float _degs = floor(latitude_dec);
    latitude_dec = (100.0 * (latitude_dec - _degs)) / 60.0;
    latitude_dec += _degs;
    // southern hemisphere is negative-valued
    if (lat == 'S') {
      latitude_dec = 0.0 - latitude_dec;
    }
    // calculate signed degree-decimal value of longitude term
    longitude_dec = longitude / 100.0;
    _degs = floor(longitude_dec);
    longitude_dec = (100.0 * (longitude_dec - _degs)) / 60.0;
    longitude_dec += _degs;
    // western hemisphere is negative-valued
    if (lon == 'W') {
      longitude_dec = 0.0 - longitude_dec;
    }
}


char MGPS::read(void) {
  char c = 0;
  
  if (paused) return c;

  if(gpsSwSerial) {
    if(!gpsSwSerial->available()) return c;
    c = gpsSwSerial->read();
  } else {
    if(!gpsHwSerial->available()) return c;
    c = gpsHwSerial->read();
  }

  //Serial.print(c);

  if (c == '$') {
    currentline[lineidx] = 0;
    lineidx = 0;
  }
  if (c == '\n') {
    currentline[lineidx] = 0;

    if (currentline == line1) {
      currentline = line2;
      lastline = line1;
    } else {
      currentline = line1;
      lastline = line2;
    }

    //Serial.println("----");
    //Serial.println((char *)lastline);
    //Serial.println("----");
    lineidx = 0;
    if (recvdflag) {
      overrunflag = true;
    }
    recvdflag = true;
  }

  currentline[lineidx++] = c;
  if (lineidx >= MAXLINELENGTH)
    lineidx = MAXLINELENGTH-1;

  return c;
}

// Constructor when using SoftwareSerial or NewSoftSerial
#if ARDUINO >= 100
MGPS::MGPS(SoftwareSerial *ser)
#else
MGPS::MGPS(NewSoftSerial *ser) 
#endif
{
  common_init();     // Set everything to common state, then...
  gpsSwSerial = ser; // ...override gpsSwSerial with value passed.
}

// Constructor when using HardwareSerial
MGPS::MGPS(HardwareSerial *ser) {
  common_init();  // Set everything to common state, then...
  gpsHwSerial = ser; // ...override gpsHwSerial with value passed.
}

// Initialization code used by all constructor types
void MGPS::common_init(void) {
  gpsSwSerial = NULL; // Set both to NULL, then override correct
  gpsHwSerial = NULL; // port pointer in corresponding constructor
  recvdflag   = false;
  overrunflag = false;
  paused      = false;
  lineidx     = 0;
  currentline = line1;
  lastline    = line2;

  hour = minute = seconds = year = month = day = fulldate =
    fixquality = satellites = 0; // uint8_t
  lat = lon = mag = 0; // char
  fix = false; // boolean
  milliseconds = 0; // uint16_t
  latitude = longitude = geoidheight = altitude =
    speed = angle = magvariation = HDOP = 0.0; // float
}

void MGPS::begin(uint16_t baud)
{
  if(gpsSwSerial) gpsSwSerial->begin(baud);
  else            gpsHwSerial->begin(baud);

  delay(10);
}

void MGPS::sendCommand(char *str) {
  if(gpsSwSerial) gpsSwSerial->println(str);
  else            gpsHwSerial->println(str);
}

boolean MGPS::newNMEAreceived(void) {
  return recvdflag;
}

boolean MGPS::overrun(void) {
  boolean temp = overrunflag;
  overrunflag = false;
  return temp;
}

void MGPS::pause(boolean p) {
  paused = p;
}

char *MGPS::lastNMEA(void) {
  recvdflag = false;
  return (char *)lastline;
}

// read a Hex value and return the decimal equivalent
uint8_t MGPS::parseHex(char c) {
    if (c < '0')
      return 0;
    if (c <= '9')
      return c - '0';
    if (c < 'A')
       return 0;
    if (c <= 'F')
       return (c - 'A')+10;
}

boolean MGPS::waitForSentence(char *wait4me, uint8_t max) {
  char str[20];

  uint8_t i=0;
  while (i < max) {
    if (newNMEAreceived()) { 
      char *nmea = lastNMEA();
      strncpy(str, nmea, 20);
      str[19] = 0;
      i++;

      if (strstr(str, wait4me))
	return true;
    }
  }

  return false;
}

/*************************************************************************
 * //Function to calculate the distance between two waypoints
 *************************************************************************/
float MGPS::distanceTo(float flat2, float flon2)
{
updateDecimalLatLon();

	float dist_calc = 0;
	float dist_calc2 = 0;
	float diflat = 0;
	float diflon = 0;

	//I've to spplit all the calculation in several steps. If i try to do it in a single line the arduino will explode.
	diflat = radians(flat2-latitude_dec);
	latitude_dec = radians(latitude_dec);
	flat2 = radians(flat2);
	diflon = radians((flon2)-(longitude_dec));
	
	dist_calc  =  (sin(diflat / 2.0)*sin(diflat / 2.0));
	dist_calc2 =  cos(latitude_dec);
	dist_calc2 *= cos(flat2);
	dist_calc2 *= sin(diflon / 2.0);
	dist_calc2 *= sin(diflon / 2.0);
	dist_calc  += dist_calc2;

	dist_calc = (2 * atan2(sqrt(dist_calc),sqrt( 1.0 - dist_calc)));

	dist_calc *= 6371000.0; //Converting to meters
	//Serial.println(dist_calc);
	return dist_calc;
}
