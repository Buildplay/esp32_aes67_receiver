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


//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP udp;
unsigned int localUdpPort = 5004; // local port to listen on
char packetBuffer[65535];         // buffer for incoming packets

unsigned int count = 0;

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
                int len = udp.read(packetBuffer, 255);

                if (len > 0)
                {
                    packetBuffer[len] = 0;
                    // count++;
                }

                ExString recvStr(packetBuffer);
                // Serial.println(String(count) + "Contents:");
                // Serial.println(ExString(recvStr.substring(32, 64)).binaryToInt());
                recv.push(recvStr);
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
        Serial.printf("Content %u: ", count);
        Serial.println(ExString(recv.unshift().substring(32, 64)).binaryToInt());
        // recv.unshift();
    }
    delayMicroseconds(250);
}