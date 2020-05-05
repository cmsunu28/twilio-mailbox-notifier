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

// Uses software serial
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// Use this for FONA 800 and 808s
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
// Use this one for FONA 3G. We didn't test that one though.
//Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

uint8_t type;

// state variable
// -1: low power mode
// 0: turned on for the first time, test RSSI
// 1: sensor went off
// 2: M2M sent successfully
int state=0;

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


}


void loop() {
  
  if (state==0) {   // enable modem sleep mode in automatic
    if (sleepModem()){
      state=1;
    }
  }
  
  if (state==1) { // needs to send the message
    if (sendM2M()) {
      delay(5000);
      state=2;
    }
    else {
      delay(1000);
    }
  }
  
  if (state==2) { // sent message! Go to sleep.
    // go to sleep
    Serial.println("Modem going to sleep.");
    if (sleepModem()) {state=3;}
  }

  if (state==3) { // Modem is asleep, now put the uC to sleep also.
      Serial.println("going to sleep");
//      sleepuC();
//      detachInterrupt(FONA_INTERRUPT);
//      state=1;
      if (digitalRead(FONA_INTERRUPT)) {
        delay(50);
        Serial.println("0");
      }
      else {
        Serial.println("1");
        state=1;
      }
  }

  delay(100);
  
}

int sleepModem() {
  // puts the FONA in sleep mode
  //  AT+CSCLK
  if (!fona.sendCheckReply(F("AT+CSCLK=2"),F("OK"))) {
    Serial.println(F("Failed to put modem in autosleep"));
    return 0;
    } else {
      Serial.println(F("Put modem in autosleep!"));
      return 1;
    }
}

int wakeModem() {
  // wakes FONA up from sleep mode
  if (!fona.sendCheckReply(F("AT+CSCLK=0"),F("OK"))) {
    Serial.println(F("Failed to wake up modem"));
    return 0;
    } else {
      Serial.println(F("Woke up modem!"));
      return 1;
    }  
}

void sleepuC() {
  // put the uC in sleep mode as well
  // Disable USB clock 
  attachInterrupt(0, FONA_INTERRUPT, LOW);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  // now you are asleep
}

int sendM2M() {
  // send an SMS!
  if (!fona.sendSMS("2936", "on")) {
    Serial.println(F("Failed to send SMS"));
    return 0;
  } else {
    Serial.println(F("Sent SMS!"));
    return 1;
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
