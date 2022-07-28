#include <Arduino.h>
#include "SPISlave.h"

/*
d7: MOSI
d6: MISO
d8: CS
d5: CLK
*/

#define DEBUGP Serial

const size_t responseSize = 17;
typedef uint8_t responseBuffer[responseSize];
const double cycle[5] = {0.00, 1.11, 2.22, 3.33, 4.44};


double fromByteSlice(const uint8_t* buf, size_t start, size_t len) {
  uint8_t doublebuf[8];
  
  for (size_t i = 0; i < len; i++) {
    doublebuf[i] = buf[start+i];
  }

  double value = *reinterpret_cast<double*>(doublebuf);
  return value;
}
double d1;
double d2;
volatile size_t cycle_idx = 0;

enum class SendMode {
  sinecos = 0,
  ladder,
  constant,
  flipper,
};

volatile SendMode sendMode = SendMode::constant;


double sineWithHarmonics(double fraction) {
  double fundamental = 2*PI*fraction;
  return sin(fundamental)*10.0 + 2.0*sin(3.0*fundamental);// +  sin(6*fundamental)*2.0;
}

double cosineWithHarmonics(double fraction) {
  double fundamental = 2*PI*fraction;
  return cos(fundamental)*10.0 + 2.0*cos(3.0*fundamental);// + cos(6*fundamental)*2.0;
}

uint8_t checksum(const uint8_t *s, size_t len){
  uint8_t c = 0;
  for (size_t i = 0;i<len;i++)
      c ^= s[i]; 
  return c;
}

void valsFromDouble(responseBuffer vals, double val1, double val2) {
  const uint8_t* ptr1 = reinterpret_cast<const uint8_t*>(&val1);
  const uint8_t* ptr2 = reinterpret_cast<const uint8_t*>(&val2);
  for (size_t i = 0; i < sizeof(double); ++i) {
      vals[i] = ptr1[i];
      vals[i+8] = ptr2[i];
  }
  vals[responseSize-1] = checksum(vals, responseSize-1);
}

uint8_t all[8] = {1};
uint8_t none[8] = {0};
double ds[2] = {fromByteSlice(all, 0, 8) + 1.1, fromByteSlice(none, 0, 8) + 2.2};
size_t flipperIdx = 0;

void setup() {
  

  Serial.begin(115200);
  Serial.println("setting up");

  // data has been received from the master. Beware that len is always 32
  // and the buffer is autofilled with zeroes if data is less than 32 bytes long
  // It's up to the user to implement protocol for handling data length
  SPISlave.onData([](uint8_t *data, size_t len) {
    String message = String((char *)data);
    (void)len;
    
    if (message.equals("return_sinecos")) {
      sendMode = SendMode::sinecos;
      Serial.println("setting mode to sinecos");
    } else if (message.equals("return_constant")) {
      Serial.println("setting mode to constant");
      sendMode = SendMode::constant;
    } else if (message.equals("return_ladder")) {
      Serial.println("setting mode to ladder");
      sendMode = SendMode::ladder;
      responseBuffer vals = {0};
      valsFromDouble(vals, cycle[cycle_idx], 0.00);
      cycle_idx = (cycle_idx + 1) % 5;
      SPISlave.setData(vals, responseSize);
    } else if (message.equals("return_flipper")) {
      Serial.println("setting mode to ladder");
      sendMode = SendMode::flipper;
      responseBuffer vals = {0};
      size_t nxtidx = (flipperIdx + 1) % 2;
      valsFromDouble(vals, ds[flipperIdx], ds[nxtidx]);
      flipperIdx = nxtidx;
      SPISlave.setData(vals, responseSize);
    }
    Serial.printf("Got data: %s\n", (char *)data);
  });

  // The master has read out outgoing data buffer
  // that buffer can be set with SPISlave.setData
  SPISlave.onDataSent([]() {
    if (sendMode == SendMode::ladder) {
      responseBuffer vals = {0};
      valsFromDouble(vals, cycle[cycle_idx], 0.00);
      cycle_idx = (cycle_idx + 1) % 5;
      SPISlave.setData(vals, responseSize);
      // DEBUGP.println("woohoo cycling");
    } 
    else if (sendMode == SendMode::flipper) {
      responseBuffer vals = {0};
      valsFromDouble(vals, ds[flipperIdx], 0.00);
      flipperIdx = (flipperIdx + 1) % 2;
      SPISlave.setData(vals, responseSize);
    }
    else if (sendMode == SendMode::sinecos) {
      responseBuffer vals = {0};
      double fraction = 60*(double)micros()/1000000.0; // 0 - 1 position in 60Hz cycle
      double v1 = sineWithHarmonics(fraction);
      double v2 = cosineWithHarmonics(fraction);
      valsFromDouble(vals, v1, v2);
      SPISlave.setData(vals, responseSize);
    }
    // DEBUGP.println("Answer Sent");
  });

  // status has been received from the master.
  // The status register is a special register that bot the slave and the master can write to and read from.
  // Can be used to exchange small data or status information
  SPISlave.onStatus([](uint32_t data) {
    Serial.printf("Status: %u\n", data);
    SPISlave.setStatus(millis());  // set next status
  });

  // The master has read the status register
  SPISlave.onStatusSent([]() {
    Serial.println("Status Sent");
  });

  // Setup SPI Slave registers and pins
  SPISlave.begin();

  // Set the status register (if the master reads it, it will read this value)
  SPISlave.setStatus(millis());

  Serial.println("setting up finished");
}



void loop() {
  
  if (sendMode == SendMode::constant) {
    responseBuffer vals;
    double val1 = 1.111;
    double val2 = 2.222;
    valsFromDouble(vals, val1, val2);
    SPISlave.setData(vals, responseSize);
  
  }
  
  delayMicroseconds(200);

}