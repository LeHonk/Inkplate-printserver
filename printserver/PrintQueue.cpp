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

#include "PrintQueue.h"
#include <SdFat.h>

static SdFat sd;

size_t PrintQueue::availableFlashSpace = 0;

void PrintQueue::updateAvailableFlashSpace() {
  availableFlashSpace = sd.freeClusterCount() * 512;
}

PrintQueue::PrintQueue(String _printerId) {
  printerId = _printerId;
}

void PrintQueue::init() {
  loadInfo();
  updateAvailableFlashSpace();
}

void PrintQueue::saveInfo() {
  SdFile infoFile = SdFile(printerId.c_str(), O_WRITE);
  infoFile.write(head);
  infoFile.write(tail);
  infoFile.close();
}

void PrintQueue::loadInfo() {
  if (sd.exists(printerId.c_str())) {
    SdFile infoFile = SdFile(printerId.c_str(), O_READ);
    head = infoFile.read();
    tail = infoFile.read();
    infoFile.close();
  } else {
    head = 0;
    tail = 0;
  }
}

void PrintQueue::startJob(int clientId) {
  head++;
  String filename = printerId + String(head);
  fileWriters[clientId] = SdFile(filename.c_str(), O_WRITE);
  saveInfo();
}

void PrintQueue::endJob(int clientId, bool cancel) {
  char fName[256];
  fileWriters[clientId].getName(fName, sizeof(fName));
  fileWriters[clientId].close();
  if (cancel) {
    if(!sd.remove(fName)) {
      Serial.print("Warning: failed to remove ");
      Serial.println(fName);
    }
  } else {
    char nName[256];
    sprintf(nName, "%sOK", fName);
    sd.rename(fName, nName);
  }
}

bool PrintQueue::canStoreByte() {
  return availableFlashSpace > 4096; //4KiB margin
}

void PrintQueue::printByte(int clientId, byte b) {
  fileWriters[clientId].write(b);
  availableFlashSpace--;
}

bool PrintQueue::hasData() {
  char fName[256];
  if (fileReader.isOpen() && fileReader.available() > 0) {
    return true;
  } else {
    if (fileReader.isOpen()) {
      fileReader.getName(fName, sizeof(fName));
      fileReader.close();
      if(!sd.remove(fName)) {
        Serial.print("Warning: failed to remove ");
        Serial.println(fName);
      }
    }
    sprintf(fName, "%s%d", printerId.c_str(), tail + 1);
    if (head == tail || sd.exists(fName)) {
      return false;
    } else {
      tail++;
      sprintf(fName, "%s%dOK", printerId.c_str(), tail);
      fileReader.open(fName, O_READ);
      saveInfo();
      return fileReader.available() > 0;
    }
  }
}

byte PrintQueue::readData() {
  return fileReader.read();
}
