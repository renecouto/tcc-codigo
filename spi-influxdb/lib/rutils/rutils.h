#ifndef RUTILSZ
#define RUTILSZ

#include <Arduino.h>

struct Measurements
{
  double v1;
  double v2;
};

double fromByteSlice(const uint8_t* buf, size_t start, size_t len) {
  uint8_t doublebuf[8];
  
  for (int i = 0; i < len; i++) {
    doublebuf[i] = buf[start+i];
  }

  double value = *reinterpret_cast<double*>(doublebuf);
  return value;
}

uint8_t checksum(const uint8_t *s, size_t len){
  uint8_t c = 0;
  for (size_t i = 0;i<len;i++)
      c ^= s[i]; 
  return c;
}


Measurements fromBytes(const uint8_t* x){
  return Measurements{fromByteSlice(x, 0, 8), fromByteSlice(x, 8, 8)};
}

#endif