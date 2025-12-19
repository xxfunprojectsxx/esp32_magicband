/*
 * ESP32-S3 MagicBand+ BLE Broadcaster with Web Interface
 * Based on information from https://emcot.world/Disney_MagicBand%2B_Bluetooth_Codes
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include <LittleFS.h>
#include <DNSServer.h>

// Access Point Configuration
const char* ap_ssid = "MagicBand-Controller";
const char* ap_password = "magicband123";

BLEAdvertising *pAdvertising;
AsyncWebServer server(80);
const byte DNS_PORT = 53;
DNSServer dnsServer;

// Forward declarations
void broadcastPacket(uint8_t* data, size_t length);
void sendPingCommand();
void sendSingleColorPalette(uint8_t color, uint8_t timing, uint8_t vibration);
void handleRoot();
void handleCommand();

// Color palette (5-bit values)
enum MagicBandColor {
  COLOR_CYAN = 0x00,
  COLOR_PURPLE = 0x01,
  COLOR_BLUE = 0x02,
  COLOR_MIDNIGHT_BLUE = 0x03,
  COLOR_BRIGHT_PURPLE = 0x05,
  COLOR_LAVENDER = 0x06,
  COLOR_PINK = 0x08,
  COLOR_YELLOW_ORANGE = 0x0F,
  COLOR_OFF_YELLOW = 0x10,
  COLOR_LIME = 0x12,
  COLOR_ORANGE = 0x13,
  COLOR_RED_ORANGE = 0x14,
  COLOR_RED = 0x15,
  COLOR_GREEN = 0x19,
  COLOR_LIME_GREEN = 0x1A,
  COLOR_WHITE = 0x1B,
  COLOR_OFF = 0x1D,
  COLOR_RANDOM = 0x1F
};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nMagicBand+ BLE Broadcaster Starting...");

  if(!LittleFS.begin(true)){  // true = format if mount fails
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  Serial.println("LittleFS mounted successfully!");
  
  // Initialize BLE
  BLEDevice::init("MB_Broadcaster");
  BLEServer *pServer = BLEDevice::createServer();
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setMinInterval(0x20);
  pAdvertising->setMaxInterval(0x40);
  
  // Set up WiFi Access Point
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  
  Serial.println("BLE Initialized.");
  Serial.println("WiFi AP Started!");
  Serial.print("SSID: ");
  Serial.println(ap_ssid);
  Serial.print("Password: ");
  Serial.println(ap_password);
  Serial.print("IP Address: ");
  Serial.println(IP);
  Serial.println("\nConnect to the WiFi network and go to:");
  Serial.print("http://");
  Serial.println(IP);

  // Start DNS server for captive portal
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  Serial.println("DNS server started for captive portal.");
  
  // Set up web server routes
  server.serveStatic("/", LittleFS, "/");
  // Redirect any unknown URL to index.html
  server.onNotFound([](AsyncWebServerRequest *request){
    // Check if it’s a captive portal detection URL
    String host = request->host();
    if (host.indexOf("clients3.google.com") != -1 ||
        host.indexOf("captive.apple.com") != -1 ||
        host.indexOf("connectivitycheck.gstatic.com") != -1) {
      request->redirect("http://" + WiFi.softAPIP().toString());
      return;
    }

    // Serve index.html for any other unknown URL
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.on("/command", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!request->hasParam("action", true)) {
        request->send(400, "text/plain", "Missing action");
        return;
    }
    String action = request->getParam("action", true)->value();
    uint8_t vib = request->hasParam("vib", true) ? request->getParam("vib", true)->value().toInt() : 0;

    Serial.print("Command: "); Serial.print(action);
    Serial.print(" Vib: "); Serial.println(vib);

    if (action == "ping") {
        uint8_t packet[] = {0xCC, 0x03, 0x00, 0x00, 0x00};
        broadcastPacket(packet, sizeof(packet));
    }
    else if (action == "preset") {
        String color = request->hasParam("color", true) ? request->getParam("color", true)->value() : "white";
        uint8_t colorCode = COLOR_WHITE;
        if (color == "red") colorCode = COLOR_RED;
        else if (color == "blue") colorCode = COLOR_BLUE;
        else if (color == "purple") colorCode = COLOR_BRIGHT_PURPLE;
        else if (color == "white") colorCode = COLOR_WHITE;
        else if (color == "green") colorCode = COLOR_GREEN;
        else if (color == "orange") colorCode = COLOR_ORANGE;
        else if (color == "cyan") colorCode = COLOR_CYAN;
        else if (color == "pink") colorCode = COLOR_PINK;

        uint8_t packet[] = {0x83,0x01,0xE9,0x05,0x00,0x2E,0x0E,(uint8_t)(0xE0|(colorCode&0x1F)),(uint8_t)(0xB0|(vib&0x0F))};
        broadcastPacket(packet, sizeof(packet));
    }
    else if (action == "rainbow") {
        uint8_t c1 = request->hasParam("c1", true) ? request->getParam("c1", true)->value().toInt() : COLOR_YELLOW_ORANGE;
        uint8_t c2 = request->hasParam("c2", true) ? request->getParam("c2", true)->value().toInt() : COLOR_RED;
        uint8_t c3 = request->hasParam("c3", true) ? request->getParam("c3", true)->value().toInt() : COLOR_GREEN;
        uint8_t c4 = request->hasParam("c4", true) ? request->getParam("c4", true)->value().toInt() : COLOR_BLUE;
        uint8_t c5 = request->hasParam("c5", true) ? request->getParam("c5", true)->value().toInt() : COLOR_PURPLE;

        uint8_t packet[] = {
            0x83,0x01,0xE9,0x09,0x00,0x2E,0x0F,
            (uint8_t)(0xA0|(c1&0x1F)),(uint8_t)(0xA0|(c2&0x1F)),
            (uint8_t)(0xA0|(c3&0x1F)),(uint8_t)(0xA0|(c4&0x1F)),(uint8_t)(0xA0|(c5&0x1F)),
            (uint8_t)(0xB0|(vib&0x0F))
        };
        broadcastPacket(packet, sizeof(packet));
    }
    else if (action == "dual") {
        uint8_t c1 = request->hasParam("c1", true) ? request->getParam("c1", true)->value().toInt() : COLOR_YELLOW_ORANGE;
        uint8_t c2 = request->hasParam("c2", true) ? request->getParam("c2", true)->value().toInt() : COLOR_BLUE;

        uint8_t packet[] = {
            0x83,0x01,0xE9,0x06,0x00,0x22,0x0F,
            (uint8_t)(0x80|(c1&0x1F)),(uint8_t)(0x80|(c2&0x1F)),
            (uint8_t)(0xB0|(vib&0x0F))
        };
        broadcastPacket(packet, sizeof(packet));
    }
    else if (action == "circle") {
        uint8_t packet[] = {
            0x83,0x01,0xE9,0x0B,0x0B,0x0F,0x0F,
            0x5C,0x5D,0x48,0xA5,0xD1,0x45,0x32,
            (uint8_t)(0xB0|(vib&0x0F))  // Changed from 0x00 to 0xB0
        };
        broadcastPacket(packet, sizeof(packet));
    }
    else if (action == "crossfade") {
        uint8_t c1 = request->hasParam("c1", true) ? request->getParam("c1", true)->value().toInt() : COLOR_RED;
        uint8_t c2 = request->hasParam("c2", true) ? request->getParam("c2", true)->value().toInt() : COLOR_BLUE;

        uint8_t packet[] = {
            0x83,0x01,0xE1,0x00,0xE9,0x11,0x00,0x6F,0x0F,
            (uint8_t)(0x40|(c1&0x1F)),(uint8_t)(0x40|(c2&0x1F)),
            0x58,0xF4,0x48,0x82,0xD1,0x46,0x02,0x08,0xD0,0x65,0x00,
            (uint8_t)(0xB0|(vib&0x0F))
        };
        broadcastPacket(packet, sizeof(packet));
    }

    request->send(200, "text/plain", "OK");
  });
  
  // when running in a captive portal, the domain is wrong, so allow CORS here
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  server.begin();
  Serial.println("Web server started!");
}

void loop() {
    dnsServer.processNextRequest();
}


void sendSingleColorPalette(uint8_t color, uint8_t timing, uint8_t vibration) {
  uint8_t packet[] = {
    0x83, 0x01,
    0xE9, 0x05,
    0x00,
    timing,
    0x0E,
    (uint8_t)(0xE0 | (color & 0x1F)),
    (uint8_t)(0xB0 | (vibration & 0x0F))
  };
  
  broadcastPacket(packet, sizeof(packet));
}

void sendPingCommand() {
  uint8_t packet[] = {
    0xCC, 0x03, 0x00, 0x00, 0x00
  };
  
  broadcastPacket(packet, sizeof(packet));
}

void broadcastPacket(uint8_t* data, size_t length) {
  pAdvertising->stop();
  delay(10);
  
  BLEAdvertisementData advData;
  
  std::string mfgData;
  mfgData += (char)0x83;
  mfgData += (char)0x01;
  
  size_t startIdx = 0;
  if(length >= 2 && data[0] == 0x83 && data[1] == 0x01) {
    startIdx = 2;
  }
  
  for(size_t i = startIdx; i < length; i++) {
    mfgData += (char)data[i];
  }
  
  advData.setManufacturerData(mfgData);
  advData.setFlags(0x06);
  
  pAdvertising->setAdvertisementData(advData);
  
  Serial.print("Broadcasting: ");
  for(size_t i = 0; i < length; i++) {
    if(data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
  }
  Serial.println();
  
  pAdvertising->start();
  delay(2000);
  pAdvertising->stop();
  delay(10);
}