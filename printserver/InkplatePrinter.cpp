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

#include "InkplatePrinter.h"

#define BYTES_BETWEEN_REFRESH 100

InkplatePrinter::InkplatePrinter(String _printerId, Inkplate* _display): Printer(_printerId) {
  display = _display;
  bytes_since_refresh = 0;
}

bool InkplatePrinter::canPrint() {
  return true;
}

void InkplatePrinter::printByte(byte b) {
  while(!canPrint()) {
    //shouldn't happen - the caller should not call this function if canWrite returned false
    Serial.println("Printer busy!");
    delay(100);
  }
  display->print(b);
  if (++bytes_since_refresh >= BYTES_BETWEEN_REFRESH) {
    display->display();
    bytes_since_refresh = 0;
  } else {
    display->partialUpdate();
  }
}

String InkplatePrinter::getInfo() {
  //TODO: Get model and stuff.
  return "Inkplate6 E-ink Printer";
}
