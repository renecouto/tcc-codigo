#ifndef CONFIGX
#define CONFIGX


// sensible configuration
const char *WIFI_SSID = "***";      // Put here your Wi-Fi SSID
const char *WIFI_PASS = "***";      // Put here your Wi-Fi password
const char *DATA_URL = "http://****:8084/api/v2/write?org=my-org&bucket=my-bucket&precision=ms";
const char *MQTT_SERVER = "****"; 
const char *AUTH_TOKEN = "Token ***";



// general configuration
const int POLL_DATA_INTERVAL_MS = 1;
const size_t BUFFER_SIZE = 1000;
const uint32_t SPI_FREQUENCY = 8 * 1000 * 1000;


#endif