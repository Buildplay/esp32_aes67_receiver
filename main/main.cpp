#include "Arduino.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "ex_string.hpp"
#include "AES67_Receiver.hpp"

unsigned short counter = 0;
AES67_Receiver recv(36); // Ring size 36 buffer: this is fast
String s = "";
TaskHandle_t th[2];

// WiFi network name and password:
const char *networkName = "WX03_Todoroki"; // "TP-LINK_62CF";
const char *networkPswd = "TodorokiWX03"; // "0484767648";

const char ssid[] = "ESP32_wifi"; // SSID
const char pass[] = "esp32pass";  // password

const IPAddress ip(192, 168, 4, 1);       // IPアドレス(ゲートウェイも兼ねる)
const IPAddress subnet(255, 255, 255, 0); // サブネットマスク

/* --- PRINTF_BYTE_TO_BINARY macro's --- */
#define PRINTF_BINARY_SEPARATOR
#define PRINTF_BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY_INT8(i) \
    (((i)&0x80ll) ? '1' : '0'),       \
        (((i)&0x40ll) ? '1' : '0'),   \
        (((i)&0x20ll) ? '1' : '0'),   \
        (((i)&0x10ll) ? '1' : '0'),   \
        (((i)&0x08ll) ? '1' : '0'),   \
        (((i)&0x04ll) ? '1' : '0'),   \
        (((i)&0x02ll) ? '1' : '0'),   \
        (((i)&0x01ll) ? '1' : '0')

#define PRINTF_BINARY_PATTERN_INT16 \
    PRINTF_BINARY_PATTERN_INT8 PRINTF_BINARY_SEPARATOR PRINTF_BINARY_PATTERN_INT8
#define PRINTF_BYTE_TO_BINARY_INT16(i) \
    PRINTF_BYTE_TO_BINARY_INT8((i) >> 8), PRINTF_BYTE_TO_BINARY_INT8(i)
#define PRINTF_BINARY_PATTERN_INT32 \
    PRINTF_BINARY_PATTERN_INT16 PRINTF_BINARY_SEPARATOR PRINTF_BINARY_PATTERN_INT16
#define PRINTF_BYTE_TO_BINARY_INT32(i) \
    PRINTF_BYTE_TO_BINARY_INT16((i) >> 16), PRINTF_BYTE_TO_BINARY_INT16(i)
#define PRINTF_BINARY_PATTERN_INT64 \
    PRINTF_BINARY_PATTERN_INT32 PRINTF_BINARY_SEPARATOR PRINTF_BINARY_PATTERN_INT32
#define PRINTF_BYTE_TO_BINARY_INT64(i) \
    PRINTF_BYTE_TO_BINARY_INT32((i) >> 32), PRINTF_BYTE_TO_BINARY_INT32(i)
/* --- end macros --- */

//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP udp;
unsigned int localUdpPort = 5004; // local port to listen on
char packetBuffer[65535];         // buffer for incoming packets

unsigned int count = 0;
char timeStr[50];
char lastStr[50];

/**
 * 
 * Need to make fetching work in core 1, and udp in core 0
 * because we cannot use VTaskDelay in microseconds order,
 * which means fetching (waiting) will block other works.
 * 
 */

void bufferFromArrival(void *pvParameters)
{
    while (true)
    {
        if (connected)
        {
            int packetSize = udp.parsePacket();
            // Serial.print(packetSize);
            if (packetSize > 0)
            {
                // read the packet into packetBufffer
                int len = udp.read(packetBuffer, 65535);

                if (len > 0)
                {
                    packetBuffer[len] = 0;
                    // count++;
                }

                // ExString recvStr(packetBuffer);
                // Serial.println(String(count) + "Contents:");
                // Serial.println(ExString(recvStr.substring(32, 64)).binaryToInt());
                // Serial.printf("Content %u: %lld :", count, esp_timer_get_time());
                // Serial.println(recvStr);
                // recvStr += ":";
                memset(timeStr, 0, 50);
                // sprintf(timeStr, "%s %lld", packetBuffer, esp_timer_get_time());
                sprintf(timeStr, "%d %lld %lld",
                    packetBuffer[12] * 256 + packetBuffer[13],
                    (long long int)packetBuffer[4] * 16777216 + packetBuffer[5] * 65535 + packetBuffer[6] * 256 + packetBuffer[7],
                    esp_timer_get_time()
                );
                // recvStr += timeStr;
                // Serial.println(recvStr);
                // Serial.println(timeStr);
                recv.push(String(timeStr));
            };
            udp.flush();
        };
    };
}

void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        //When connected set
        Serial.print("WiFi connected! IP address: ");
        Serial.println(WiFi.localIP());
        connected = true;
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        connected = false;
        break;
    default:
        break;
    }
}

void printWifiStatus()
{
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

void connectToWiFi(const char *ssid, const char *pwd)
{
    Serial.println("Connecting to WiFi network: " + String(ssid));

    // delete old config
    WiFi.disconnect(true);
    //register event handler
    WiFi.onEvent(WiFiEvent);

    //Initiate connection
    WiFi.begin(ssid, pwd);

    Serial.print("Waiting for WIFI connection ");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(". ");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    printWifiStatus();
}

void setupAccessPoint()
{
    WiFi.softAP(ssid, pass);           // SSIDとパスの設定
    delay(100);                        // 追記：このdelayを入れないと失敗する場合がある
    WiFi.softAPConfig(ip, ip, subnet); // IPアドレス、ゲートウェイ、サブネットマスクの設定

    Serial.print("AP IP address: ");
    IPAddress myIP = WiFi.softAPIP();
    Serial.println(myIP);

    connected = true;
}

void connectToSerial(unsigned int baudRate)
{
    // Initilize hardware serial:
    Serial.begin(baudRate);

    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB port only
    }
}

void setup()
{
    connectToSerial(115200);
    Serial.println(" ");
    
    // connectToWiFi(networkName, networkPswd);
    setupAccessPoint();
    udp.begin(localUdpPort);
    
    xTaskCreatePinnedToCore(
        bufferFromArrival,
        "loop for buffering packets",
        4096,
        NULL,
        4,
        &th[0],
        0
    );
}

void loop()
{
    if (recv.currentBufferLength() >= 20) // NOT GOOD: when stream packet stops, fetching will stop fast.
    {
        // Serial.print("> ");
        // Serial.println(recv.unshift());
        count++;
        String str = recv.unshift();
        // if (count % 100 == 0) {
            // Serial.printf("Content %u: ", count);
            // Serial.printf("Content %u: ", count);
            // Serial.println(ExString(str.substring(32, 64)).binaryToInt());
            // Serial.println(str.substring(289));
            // Serial.print(str);
            // Serial.printf(":%lld\n", esp_timer_get_time());

            memset(lastStr, 0, 50);
            // sprintf(lastStr, "Content %u: %lld", count, esp_timer_get_time());
            // sprintf(lastStr, "Content %u: %s", count, str.c_str());
            sprintf(lastStr, "Content %u: %s %lld", count, str.c_str(), esp_timer_get_time());
            Serial.println(lastStr);
            // };
            // recv.unshift();
    }
    delayMicroseconds(250);

    // delay(20);
}