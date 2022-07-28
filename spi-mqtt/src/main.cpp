
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
uint64_t firstMillis;

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
  return ((uint64_t)t2*(uint64_t)1000) -(3*3600*1000);
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

bool IRAM_ATTR TimerHandler0(void* timerNo)
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

  if (ITimer0.attachInterruptInterval(POLL_DATA_INTERVAL_MS * 1000, TimerHandler0))
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
  firstMillis = millis();
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
  client.setBufferSize(50*BUFFER_SIZE);
  client.setCallback(callback);
  setInterrupt();
}

void sendData2(uint64_t currentTime) {
  char *dataPoints = (char *) malloc(60*BUFFER_SIZE*sizeof(char));
  uint32_t written = formatDataPoints4(dataPoints, buf,BUFFER_SIZE, currentTime, POLL_DATA_INTERVAL_MS);

  Serial.printf("trying to send data of length %d\n", written);
  if(WiFi.status() == WL_CONNECTED || (wifiMulti.run(10000) == WL_CONNECTED)) {
    
    
    uint64_t before = millis();
    
    // int httpCode = http.POST((uint8_t *) dataPoints, written);
    bool ret = client.publish("rust/mqtt", (uint8_t*)dataPoints, written);
    uint64_t after = millis();
    if (ret) {
      Serial.printf("result from publish was: true. took %d Millis\n", after - before);
    } else {
      Serial.println("result from publish was: false");
    }
    
  }
  free(dataPoints);
}


uint64_t getCurrentTime() {
  return currentTime + (uint64_t)millis() - (uint64_t)firstMillis;
}

int loopCounter = 0;

void loop()
{
  delay(1);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  loopCounter++;
  if(loopCounter%1000 ==0) {
    Serial.println("looping...");
  }
  if (dataIsReady) {

    sendData2(getCurrentTime());
    cur_pos = 0;
    dataIsReady = false;
    if (!stopInterrupt){
      ITimer0.setInterval(POLL_DATA_INTERVAL_MS *1000, TimerHandler0);
    }
  }
  
}