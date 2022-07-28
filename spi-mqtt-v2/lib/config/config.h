#ifndef CONFIGX
#define CONFIGX


const char *WIFI_SSID = "***";      // Put here your Wi-Fi SSID
const char *WIFI_PASS = "***";      // Put here your Wi-Fi password


const int POLL_DATA_INTERVAL_US = 500;
const size_t BUFFER_SIZE = 2000;
const uint32_t SPI_FREQUENCY = 8 * 1000 * 1000;
const char *DATA_URL = "http://192.168.0.29:8084/api/v2/write?org=my-org&bucket=my-bucket&precision=ms";

const char *MQTT_SERVER = "192.168.0.29";
const char *AUTH_TOKEN = "Token ***";


#endif