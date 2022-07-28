
/*
   Notes:
   Special design is necessary to share data between interrupt code and the rest of your program.
   Variables usually need to be "volatile" types. Volatile tells the compiler to avoid optimizations that assume
   variable can not spontaneously change. Because your function may change variables while your program is using them,
   the compiler needs this hint. But volatile alone is often not enough.
   When accessing shared variables, usually interrupts must be disabled. Even with volatile,
   if the interrupt changes a multi-byte variable between a sequence of instructions, it can be read incorrectly.
   If your data is multiple variables, such as an array and a count, usually interrupts need to be disabled
   or the entire sequence of your code which accesses the data.
*/

#ifndef ESP32
  #error This code is designed to run on ESP32 platform, not Arduino nor ESP8266! Please check your Tools->Board setting.
#endif


#include <Arduino.h>
#define USE_SERIAL Serial
#include <ESP32TimerInterrupt.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>
#include <WiFiUdp.h>

#include "config.h"
#include <formatData.h>
#include <spiSafeMaster.h>
#include "time.h"
#include "sntp.h"
#include <rutils.h>

#ifndef LED_BUILTIN
  #define LED_BUILTIN       2         // Pin D2 mapped to pin GPIO2/ADC12 of ESP32, control on-board LED
#endif

// Don't use PIN_D1 in core v2.0.0 and v2.0.1. Check https://github.com/espressif/arduino-esp32/issues/5868
#define PIN_D1              1         // Pin D1 mapped to pin GPIO1 of ESP32-S2
#define PIN_D2              2         // Pin D2 mapped to pin GPIO2/ADC12/TOUCH2/LED_BUILTIN of ESP32
#define PIN_D3              3         // Pin D3 mapped to pin GPIO3/RX0 of ESP32


// unsigned long timer;
// control variables for filling buffer
volatile int cur_pos = 0;
volatile bool dataIsReady = false;
volatile Measurements buf[BUFFER_SIZE] = {};
WiFiUDP Udp; // for network time
unsigned int localPort = 8888;  // local port to listen for UDP packets
uint64_t currentTime;
uint64_t firstMicros;

bool timeIsAvailable = false;
uint64_t getTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("No time available (yet)");
    return 0;
  }
  unsigned long t2 = mktime(&timeinfo);
  timeIsAvailable = true;
  return ((uint64_t)t2*(uint64_t)1000*1000) -((uint64_t)3*3600* (uint64_t)1000000);
}


double get_sine(int i) {
  double fundamental = 2*PI*i/BUFFER_SIZE;
  return sin(fundamental)*10 + sin(3*fundamental)*1 + sin(11*fundamental)*0.5;
}


ESP32Timer ITimer0(0);

WiFiMulti wifiMulti;
WiFiClient espWifiClient;
PubSubClient client(espWifiClient);

ESPSafeMaster esp(SS);


bool IRAM_ATTR Nothing(void* timerNo) {
  return true;
}



Measurements receive() {
  uint8_t databuf[32];
  esp.readData(databuf);
  if (databuf[16] != checksum(databuf, 16)) {
    return Measurements{1.234567, 1.234567}; // how to propagate errors?
  }
  return fromBytes(databuf);
}

bool IRAM_ATTR TimerHandler2(void* timerNo)
{
  if (dataIsReady) return true;
  const Measurements x = receive();
  buf[cur_pos].v1 = x.v1;
  buf[cur_pos].v2 = x.v2;
  cur_pos += 1;
  if (cur_pos == BUFFER_SIZE) {
    dataIsReady = true;  
  }
  
  return true;  
}

volatile bool bufready1 = false;
volatile bool bufready2 = false;
volatile Measurements buf1[BUFFER_SIZE] = {};
volatile Measurements buf2[BUFFER_SIZE] = {};
volatile size_t bufidx1 = 0;
volatile size_t bufidx2 = 0;

bool IRAM_ATTR TimerHandler0(void* timerNo)
{
  if (!bufready1) {
    const Measurements x = receive();
    buf1[bufidx1].v1 = x.v1;
    buf1[bufidx1].v2 = x.v2;
    bufidx1 += 1;
    if (bufidx1 == BUFFER_SIZE) {
      bufready1 = true;  
    }
    
    return true;  
  } else if (!bufready2) {
    const Measurements x = receive();
    buf2[bufidx2].v1 = x.v1;
    buf2[bufidx2].v2 = x.v2;
    bufidx2 += 1;
    if (bufidx2 == BUFFER_SIZE) {
      bufready2 = true;  
    }
    return true;  
  } else {
    return true;
  }
  
}

void timeavailable(struct timeval *t)
{
  timeIsAvailable = true;
}


void connectToWifi() {
  wifiMulti.addAP(WIFI_SSID, WIFI_PASS);
  WiFi.mode(WIFI_STA);
  while (wifiMulti.run(10000) != WL_CONNECTED);
  Serial.println("connected to wifi");
}

bool slaveHealthCheck() {
  const char* returnConstant = "return_constant";
  const char* returnLadder = "return_ladder";
  const char* returnFlipper = "return_flipper";
  const char* returnSinCos = "return_sinecos";
  uint32_t status = esp.readStatus();
  Serial.printf("status from slave was: %d\n", status);
  esp.writeData(returnConstant);
  delay(1);
  Measurements m = receive();
  Serial.printf("Expected 1.111 and 2.222, got: %.5f and %.5f\n", m.v1, m.v2);

  delay(1);
  esp.writeData(returnLadder);
  delay(1);
  Measurements ms[5];
  for (size_t i = 0; i < 5; i++) {
    ms[i] = receive();
    delay(1);
  }
  Serial.printf("Expected 0.00, 1.11, 2.22, 3.33, 4.44, got: %.5f, %.5f, %.5f, %.5f, %.5f\n", ms[0].v1, ms[1].v1, ms[2].v1, ms[3].v1, ms[4].v1);

  esp.writeData(returnSinCos);
  delay(1);
  
  return false;
}

void setInterrupt() {

  if (ITimer0.attachInterruptInterval(POLL_DATA_INTERVAL_US, TimerHandler0))
  {
    Serial.print(F("Starting  ITimer0 OK, millis() = ")); 
    Serial.println(millis());
  }
  else
    Serial.println(F("Can't set ITimer0. Select another freq. or timer"));
}

void waitForRemoteTime() {
  Serial.println("waiting for local time");
  currentTime = getTime();
  
  while(!timeIsAvailable) { delay(10); };
  currentTime = getTime();
  Serial.println("got local time");
  firstMicros = micros();
}

bool stopInterrupt = false;

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    // Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);
  if(messageTemp == "restart") {
    Serial.println("restarting due to message");
    delay(5000);
    ESP.restart();
  } else if(messageTemp == "turn-off-interrupt") {
    Serial.println("turning off interrupt and data sending");
    ITimer0.detachInterrupt();
    stopInterrupt = true;
  } else if(messageTemp == "turn-on-interrupt") {
    Serial.println("turning on interrupt and data sending");
    setInterrupt();
    stopInterrupt = false;
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_D3,      OUTPUT);

  Serial.begin(115200);
  while (!Serial);

  delay(100);

  Serial.print(F("\nStarting on ")); Serial.println(ARDUINO_BOARD);
  Serial.print(F("CPU Frequency = ")); Serial.print(F_CPU / 1000000); Serial.println(F(" MHz"));
  
  sntp_set_time_sync_notification_cb(timeavailable);
  configTime(0, 0, "br.pool.ntp.org");
  Serial.println("configured time");
  Serial.println("connecting to wifi");

  connectToWifi();

  waitForRemoteTime();
  SPI.setFrequency(SPI_FREQUENCY); // 8 Mbit/s
  SPI.begin();
  esp.begin();
  delay(1);
  bool error = slaveHealthCheck();
  if (error) {
    Serial.println("error doing spi contant");
  }
  client.setServer(MQTT_SERVER, 1883);
  client.setBufferSize(40000);
  client.setCallback(callback);
  setInterrupt();
}

void sendData2(uint64_t currentTime, volatile Measurements* buf) {
  // return;
  Serial.println("before aloc");
  Serial.flush();
  delay(1000);
  char* dataPoints = (char *) malloc(100*BUFFER_SIZE*sizeof(char));//60*BUFFER_SIZE*sizeof(char));
  // char* end = dataPoints;
  int w = 0;

  Serial.println("after aloc");
  Serial.flush();
  delay(1000);

  for (size_t i = 0; i < BUFFER_SIZE; i++) {
    
    uint64_t n = (currentTime - (BUFFER_SIZE -1 -i)*POLL_DATA_INTERVAL_US);
    double v1 = buf[i].v1;
    double v2 =  buf[i].v2;
    if (v1 != 1.234567 && v2 != 1.234567 && v1 != 0.0 && v2 != 0.0) {
      w += sprintf(dataPoints + w, "%.4f,%.4f,%llu\n", v1,v2, n);
    }
    // Serial.println(dataPoints);
    // delay(1000);
    // end = stpcpy(end, l);
  }
  // Serial.println(dataPoints);
  // Serial.flush();
  // free(dataPoints);
  // return;
  // uint32_t written = formatDataPoints5(dataPoints, buf,BUFFER_SIZE, currentTime, POLL_DATA_INTERVAL_US);

  Serial.printf("trying to send data of length %d\n", w);
  if(WiFi.status() == WL_CONNECTED || (wifiMulti.run(10000) == WL_CONNECTED)) {
    
    
    uint64_t before = millis();
    
    // int httpCode = http.POST((uint8_t *) dataPoints, written);
    int32_t chunkSize = 4000;
    boolean ret = client.beginPublish("rust/mqtt", w, false);
    for (int32_t i =0; ; i ++){
      int32_t ends = min(w- i*chunkSize, chunkSize);
      if(ends <= 0) {
        break;
      }
      client.write((uint8_t*)dataPoints+i*chunkSize, ends);
    }
    client.endPublish();
      
    // bool ret = client.publish("rust/mqtt", (uint8_t*)dataPoints, w);
    uint64_t after = millis();
    if (ret) {
      Serial.printf("result from publish was: true. took %d Millis\n", after - before);
    } else {
      Serial.println("result from publish was: false");
    }
    
  }
  free(dataPoints);
}

// void sendData2(uint64_t currentTime, volatile Measurements* buf) {
//   char dataPoints[UINT16_MAX] = "";//(char *) malloc(UINT16_MAX);//60*BUFFER_SIZE*sizeof(char));
//   String res(dataPoints);
//   formatDataPoints5(res, buf,BUFFER_SIZE, currentTime, POLL_DATA_INTERVAL_US);

//   Serial.printf("trying to send data of length unknown\n");
//   if(WiFi.status() == WL_CONNECTED || (wifiMulti.run(10000) == WL_CONNECTED)) {
    
    
//     uint64_t before = millis();
    
//     // int httpCode = http.POST((uint8_t *) dataPoints, written);
//     bool ret = client.publish("rust/mqtt", (uint8_t*)res.c_str(), strlen(res.c_str()));
//     uint64_t after = millis();
//     if (ret) {
//       Serial.printf("result from publish was: true. took %d Millis\n", after - before);
//     } else {
//       Serial.println("result from publish was: false");
//     }
    
//   }
//   // free(dataPoints);
// }


// void sendData3(uint64_t currentTime, volatile Measurements* buf) {
//   char dataPoints[200] = "";
  
//   if(WiFi.status() == WL_CONNECTED || (wifiMulti.run(10000) == WL_CONNECTED)) {
    
    
//     uint64_t before = millis();
//     client.beginPublish();
//     for (size_t i = 0; i< BUFFER_SIZE; i++){
//       uint32_t written = formatDataLine(dataPoints, buf,BUFFER_SIZE, currentTime, POLL_DATA_INTERVAL_US);
//     }
    

//     // Serial.printf("trying to send data of length %d\n", written);
    
//     // int httpCode = http.POST((uint8_t *) dataPoints, written);
//     bool ret = client.publish("rust/mqtt", (uint8_t*)dataPoints, written);
//     uint64_t after = millis();
//     if (ret) {
//       Serial.printf("result from publish was: true. took %d Millis\n", after - before);
//     } else {
//       Serial.println("result from publish was: false");
//     }
    
//   }
//   free(dataPoints);
// }


uint64_t getCurrentTime() {
  return currentTime + (uint64_t)micros() - (uint64_t)firstMicros;
}

int loopCounter = 0;

void loop()
{
//  delay(100);
//  Serial.println(nowz);
//  return;
  delay(1);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  loopCounter++;
  if(loopCounter%1000 ==0) {
    Serial.println("looping...");
  }
  if (bufready1) {
    sendData2(getCurrentTime(), buf1);
    bufidx1 = 0;
    bufready1 = false;
  }

  if (bufready2) {
    sendData2(getCurrentTime(), buf2);
    bufidx2 = 0;
    bufready2 = false;
  }
  
}