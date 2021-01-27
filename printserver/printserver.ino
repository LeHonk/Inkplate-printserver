/*
    This file is part of printserver-esp8266.

    printserver-esp8266 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    printserver-esp8266 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with printserver-esp8266.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Inkplate.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <SdFat.h>
#include <ESPmDNS.h>

#include "TcpPrintServer.h"
#include "PrintQueue.h"
#include "InkplatePrinter.h"

#include "ssid_pass.h"

Inkplate display(INKPLATE_1BIT);

InkplatePrinter printer("inkplate", &display);

Printer* printers[] = {&printer};

#define PRINTER_COUNT (sizeof(printers) / sizeof(printers[0]))
TcpPrintServer server(printers, PRINTER_COUNT);

void setup() {
  Serial.begin(115200);
  Serial.println("boot ok");

  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid, password);
  
  sd.begin();
  for (unsigned int i = 0; i < PRINTER_COUNT; i++) {
    printers[i]->init();
  }
  Serial.println("initialized printers");

  // Set up mDNS responder
  if (!MDNS.begin("Inkplate6")) {
    Serial.println("Error setting up MDNS responder!");
    while(1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  MDNS.addService("_ipp", "_tcp", IPP_SERVER_PORT);
  MDNS.addServiceTxt("_ipp","_tcp", "PaperMax", "<legal-A4");
  MDNS.addServiceTxt("_ipp","_tcp", "UUID", "3bdaf54c-5d6c-11eb-8b82-0026bb64bada");
  MDNS.addServiceTxt("_ipp","_tcp", "note", "basement"); // Placement
  MDNS.addServiceTxt("_ipp","_tcp", "pdl", "text/plain");
  MDNS.addServiceTxt("_ipp","_tcp", "rp", "inkplate");  // TODO One per printer?
  MDNS.addServiceTxt("_ipp","_tcp", "qtotal", "1");
  MDNS.addServiceTxt("_ipp","_tcp", "txtVers", "1");
  //MDNS.setInstanceName("_print._sub");

  server.start();

  display.begin();
  display.clearDisplay();
  display.display();
  display.setTextColor(BLACK, WHITE);
  display.setTextSize(2);
  display.setTextWrap(true);
  
  Serial.println("setup ok");
}

void loop() {
  printDebugAndYield();
  server.process();
  for (unsigned int i = 0; i < PRINTER_COUNT; i++) {
    printers[i]->processQueue();
  }
}

inline void printDebugAndYield() {
  static unsigned long lastCall = 0;
  if (millis() - lastCall > 5000) {
    Serial.printf("Free heap: %d bytes\r\n", ESP.getFreeHeap());
    server.printInfo();
    //Serial.printf("[Filesystem] Total bytes: %d, Used bytes: %d\r\n", SPIFFS.totalBytes(), SPIFFS.usedBytes());

    PrintQueue::updateAvailableFlashSpace();
    yield();

    lastCall = millis();
  }
}
