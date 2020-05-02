#include <LowPower.h>

/***************************************************
  This is an example for our Adafruit FONA Cellular Module

  Designed specifically to work with the Adafruit FONA
  ----> http://www.adafruit.com/products/1946
  ----> http://www.adafruit.com/products/1963
  ----> http://www.adafruit.com/products/2468
  ----> http://www.adafruit.com/products/2542

  These cellular modules use TTL Serial to communicate, 2 pins are
  required to interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

/*
THIS CODE IS STILL IN PROGRESS!

Open up the serial console on the Arduino at 115200 baud to interact with FONA

Note that if you need to set a GPRS APN, username, and password scroll down to
the commented section below at the end of the setup() function.
*/
#include <Adafruit_FONA.h>


#define FONA_RX 9
#define FONA_TX 8
#define FONA_RST 4
#define FONA_RI 7

#define FONA_INTERRUPT 0

char replybuffer[255];

// We default to using software serial. If you want to use hardware serial
// (because softserial isnt supported) comment out the following three lines 
// and uncomment the HardwareSerial line
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// Hardware serial is also possible!
//  HardwareSerial *fonaSerial = &Serial1;

// Use this for FONA 800 and 808s
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
// Use this one for FONA 3G
//Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

uint8_t type;

// state variable
// -1: low power mode
// 0: sensor went off
// 1: sensor went off
// 2: M2M sent successfully
int state=0;

int waitTime=10000; // wait for a while for the SMS feature to boot up before you try to send a text
int startTime=0;

void setup() {

  pinMode(FONA_INTERRUPT,INPUT);
  
  // set up FONA: code from Adafruit
  Serial.begin(115200);
  Serial.println(F("FONA basic test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  type = fona.type();
  Serial.println(F("FONA is OK"));
  Serial.print(F("Found "));
  switch (type) {
    case FONA800L:
      Serial.println(F("FONA 800L")); break;
    case FONA800H:
      Serial.println(F("FONA 800H")); break;
    case FONA808_V1:
      Serial.println(F("FONA 808 (v1)")); break;
    case FONA808_V2:
      Serial.println(F("FONA 808 (v2)")); break;
    case FONA3G_A:
      Serial.println(F("FONA 3G (American)")); break;
    case FONA3G_E:
      Serial.println(F("FONA 3G (European)")); break;
    default: 
      Serial.println(F("???")); break;
  }
  
  // Print module IMEI number.
  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("Module IMEI: "); Serial.println(imei);
  }
  
  // set APN
  fona.setGPRSNetworkSettings(F("wireless.twilio.com"));

  startTime=millis();

}


void loop() {
  if (state==0) {
    if (millis()-startTime>waitTime) {
        if (getRSSI()>10) {
          state=1;    
        }
    }
  }
  if (state==1) {
    sendM2M();
    delay(50);
  }
  if (state==2) {
    // go to sleep
    sleepMode();
  }

  if (state==3) {
    // modem is asleep
    // put the uC asleep as well
    if (millis()-startTime>waitTime) {
      // put the uC in sleep mode as well
      // Disable USB clock 
      attachInterrupt(0, FONA_INTERRUPT, HIGH);
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
      // now you are asleep
      // this does something weird to the serial monitor
      detachInterrupt(0);
      state=0;
    }
  }
}

void readSensor() {
  // reads digital sensor and returns 0 or 1
  startTime=millis();
}

void sleepMode() {
  // puts the FONA in sleep mode
  //  AT+CSCLK
  startTime=millis();
  if (!fona.sendCheckReply(F("AT+CSCLK=1"),F("OK"))) {
    Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
      state=3;
    }
}

void sendM2M() {
  // send an SMS!
  if (!fona.sendSMS("2936", "on")) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("Sent!"));
    state=2;
  }
}

int getRSSI() {
  // read the RSSI
  uint8_t n = fona.getRSSI();
  int8_t r;
  
  Serial.print(F("RSSI = ")); Serial.print(n); Serial.print(": ");
  if (n == 0) r = -115;
  if (n == 1) r = -111;
  if (n == 31) r = -52;
  if ((n >= 2) && (n <= 30)) {
    r = map(n, 2, 30, -110, -54);
  }
  Serial.print(r); Serial.println(F(" dBm"));

  return n;
}

void readBatt() {
  // read the battery voltage and percentage
  uint16_t vbat;
  if (! fona.getBattVoltage(&vbat)) {
    Serial.println(F("Failed to read Batt"));
  } else {
    Serial.print(F("VBat = ")); Serial.print(vbat); Serial.println(F(" mV"));
  }

  if (! fona.getBattPercent(&vbat)) {
    Serial.println(F("Failed to read Batt"));
  } else {
    Serial.print(F("VPct = ")); Serial.print(vbat); Serial.println(F("%"));
  }
}
