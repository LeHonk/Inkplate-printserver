#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <map>
#include "Settings.h"
#include "TcpStream.h"
#include "Printer.h"

class TcpPrintServer {
  private:
    WiFiServer socketServer;
    WiFiServer ippServer;
    WiFiServer httpServer;
    TcpStream* clients[MAXCLIENTS];
    Printer* printer;

    void handleClient(int index);

    void processSocketClients();
    void processIppClients();
    void processWebClients();
  public:
    TcpPrintServer(Printer* p);
    void start();
    void process();
    void printInfo();
};
