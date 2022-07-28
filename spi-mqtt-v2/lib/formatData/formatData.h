#ifndef FORMAT_DATA_POINTS
#define FORMAT_DATA_POINTS

#include <Arduino.h>
#include <rutils.h>
#include <config.h>
#include <string.h>

String formatDataPoints2(volatile Measurements* buff, size_t buffSize, uint64_t currentMillis, uint32_t pointMillisInterval) {
  /*
   * return String, Stream or bytearray? or char?
   * - format using sprintf, statically dimension buffer size
   */
   String tpl = "";
   for (size_t i = 0; i < buffSize; i++) {
      char zb[200];
      uint64_t n = (currentMillis - (buffSize -1 -i)*pointMillisInterval); // assume 1 mili para cada ponto. devemos remover i*pointMillisInterval do tempo de cada ponto
      double v1 = buff[i].v1;
      double v2 = buff[i].v2;
      if (v1 != 1.234567 && v2 != 1.234567 && v1 != 0.0 && v2 != 0.0) {
         sprintf(zb, "test2 variable1=%.4f,variable2=%.4f %llu\n", v1, v2, n);
      } else {
         sprintf(zb, "");
      }
      tpl = tpl + zb;
   }
   return tpl;
}


uint32_t formatDataPoints3(char* tpl, volatile Measurements* buff, size_t buffSize, uint64_t currentMillis, uint32_t pointMillisInterval) {
   char* end = tpl;
   uint32_t w = 0;
   for (size_t i = 0; i < buffSize; i++) {
      char zb[200];
      uint64_t n = (currentMillis - (buffSize -1 -i)*pointMillisInterval); // assume 1 mili para cada ponto. devemos remover i*pointMillisInterval do tempo de cada ponto
      double v1 = buff[i].v1;
      double v2 = buff[i].v2;
      if (v1 != 1.234567 && v2 != 1.234567 && v1 != 0.0 && v2 != 0.0) {
         int wz = sprintf(zb, "test2 variable1=%.4f,variable2=%.4f %llu\n", v1, v2, n);
         w += wz;
      } else {
         sprintf(zb, "");
      }
      end = stpcpy(end, zb);
   }
   return w;
}

uint32_t formatDataPoints4(char* tpl, volatile Measurements* buff, size_t buffSize, uint64_t currentMillis, uint32_t pointMillisInterval) {
   char* end = tpl;
   uint32_t w = 0;
   for (size_t i = 0; i < buffSize; i++) {
      char zb[200];
      uint64_t n = (currentMillis - (buffSize -1 -i)*pointMillisInterval); // assume 1 mili para cada ponto. devemos remover i*pointMillisInterval do tempo de cada ponto
      double v1 = buff[i].v1;
      double v2 = buff[i].v2;
      if (v1 != 1.234567 && v2 != 1.234567 && v1 != 0.0 && v2 != 0.0) {
         int wz = sprintf(zb, "%.4f,%.4f,%llu\n", v1, v2, n);
         w += wz;
      } else {
         sprintf(zb, "");
      }
      end = stpcpy(end, zb);
   }
   return w;
}

uint32_t formatDataPoints5(char* tpl, volatile Measurements* buff, size_t buffSize, uint64_t currentMicros, uint32_t pointMicrosInterval) {
   char* end = tpl;
   uint32_t w = 0;
   for (size_t i = 0; i < buffSize; i++) {
      // Serial.println(tpl);
      // Serial.flush();
      // delay(100);
      char zb[200] = "";
      uint64_t n = (currentMicros - (buffSize -1 -i)*pointMicrosInterval); // assume 1 mili para cada ponto. devemos remover i*pointMillisInterval do tempo de cada ponto
      double v1 = buff[i].v1;
      double v2 = buff[i].v2;
      if (v1 != 1.234567 && v2 != 1.234567 && v1 != 0.0 && v2 != 0.0) {
         int wz = sprintf(zb, "%.4f,%.4f,%llu\n", v1, v2, n);
         w += wz;
      }
      
      end = stpcpy(end, zb);
   }
   return w;
}

String formatDataPoints6(String tpl, volatile Measurements* buff, size_t buffSize, uint64_t currentMicros, uint32_t pointMicrosInterval) {
   // String tpl;
   // bool res = tpl.reserve(UINT16_MAX);
   // if (!res) {
   //    Serial.println("failed to reserve!!!");
   // }
   // uint32_t w = 0;
   for (size_t i = 0; i < buffSize; i++) {
      // Serial.println(tpl);
      // Serial.flush();
      // delay(100);
      char zb[200] = "";
      uint64_t n = (currentMicros - (buffSize -1 -i)*pointMicrosInterval); // assume 1 mili para cada ponto. devemos remover i*pointMillisInterval do tempo de cada ponto
      double v1 = buff[i].v1;
      double v2 = buff[i].v2;
      if (v1 != 1.234567 && v2 != 1.234567 && v1 != 0.0 && v2 != 0.0) {
         int wz = sprintf(zb, "%.4f,%.4f,%llu\n", v1, v2, n);
         // w += wz;
         
         tpl += zb;
      }
      
      // end = stpcpy(end, zb);
   }
   return tpl;
}

uint32_t formatDataLine(char* zb, volatile Measurements ms, size_t i, size_t buffSize, uint64_t currentMicros, uint32_t pointMicrosInterval) {
   uint64_t n = (currentMicros - (buffSize -1 -i)*pointMicrosInterval); // assume 1 mili para cada ponto. devemos remover i*pointMillisInterval do tempo de cada ponto
   if (ms.v1 != 1.234567 && ms.v2 != 1.234567 && ms.v1 != 0.0 && ms.v2 != 0.0) {
      return sprintf(zb, "%.4f,%.4f,%llu\n", ms.v1, ms.v2, n);
      
   }
   return 0;
}




#endif