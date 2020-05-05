#include <LowPower.h>

#include <Adafruit_FONA.h>
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

#define FONA_RX 9
#define FONA_TX 8
#define FONA_RST 4
#define FONA_RI 7

#define FONA_INTERRUPT 2 // "Interrupt pin #2" is actually GPIO 0
int DTR_CONTROL=5; // According to the pinout, this is our DTR pin

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
// -1: just booted up, configure modem
// 0: waiting for mail (light)
// 1: sensor went off (saw the light!)
int state=-1;
// should be -1 on boot, then go between 0 and 1.

//int lightPin=0;

void setup() {
//  pinMode(lightPin,INPUT); // this will be the interrupt
  pinMode(DTR_CONTROL,OUTPUT); // this controls the modem
  digitalWrite(DTR_CONTROL,LOW); // Make sure it starts with the modem on!
  delay(5000);

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
  
  if (state==-1) {   // enable modem sleep mode in automatic
    if (sleepModem()){
      digitalWrite(DTR_CONTROL,HIGH); // puts modem to sleep manually
      delay(500);
      state=0;
    }
  }

  if (state==0) {
    
    // Enter power down state with ADC and BOD module disabled.
    // Wake up when wake up pin is low.
    Serial.println("going to sleep");
    attachInterrupt(2,wakeUp,FALLING);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
    
  }
  
  if (state==1) { // send the message
    if (getRSSI()>9) {
      if (sendM2M()) {
        delay(5000);
        state=0;
      }      
    }
    else {
      delay(3000);
    }
  }
  
}

int sleepModem() {
//   puts the FONA in sleep mode
//    AT+CSCLK
  if (!fona.sendCheckReply(F("AT+CSCLK=1"),F("OK"))) {
    Serial.println(F("Changing modem to DTR control"));
    return 0;
    } else {
      Serial.println(F("Modem put in DTR control"));
      return 1;
    }
}

void wakeUp() {
  Serial.println("woke up!");
  digitalWrite(DTR_CONTROL,LOW); // wakes modem up manually
  delay(500); // give it half a second to wake up
  state=1;
}

int sendM2M() {
  // send an SMS!
  if (!fona.sendSMS("2936", "on")) {
    Serial.println(F("Failed to send SMS"));
    return 0;
  } else {
    Serial.println(F("Sent SMS!"));
    state=2;
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
