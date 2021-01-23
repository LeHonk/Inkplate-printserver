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

#pragma once
#include "Printer.h"
#include <Inkplate.h>

class InkplatePrinter: public Printer {
  private:
    Inkplate* display;
    unsigned int bytes_since_refresh;
  protected:
    bool canPrint();
    void printByte(byte b);
  public:
    InkplatePrinter(String _printerId, Inkplate* _display);
    String getInfo();
};
