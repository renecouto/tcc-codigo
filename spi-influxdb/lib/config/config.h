#ifndef CONFIGX
#define CONFIGX


const char *WIFI_SSID = "***";      // Put here your Wi-Fi SSID
const char *WIFI_PASS = "***";      // Put here your Wi-Fi password



// const int PUBLISH_INTERVAL_MS = 5000;
const int POLL_DATA_INTERVAL_MS = 1;
const size_t BUFFER_SIZE = 1000;
const uint32_t SPI_FREQUENCY = 8 * 1000 * 1000;
///const char *DATA_URL = "http://192.168.100.5:8086/api/v2/write?org=my-org&bucket=my-bucket&precision=ms";
const char *DATA_URL = "http://192.168.100.5:8084/api/v2/write?org=my-org&bucket=my-bucket&precision=ms";
// const char *DATA_URL = "http://172.20.10.2:8084/api/v2/write?org=my-org&bucket=my-bucket&precision=ms";
const char *AUTH_TOKEN = "Token ***";


#endif