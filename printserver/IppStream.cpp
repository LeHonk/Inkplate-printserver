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

#include <WiFi.h>
#include "Settings.h"
#include "IppStream.h"

#include "inkplate6_png.h"

std::set<String> allPrinterDescriptionAttributes {
  "charset-configured",
  "charset-supported",
  "color-supported",
  "copies-default",
  "copies-supported",
  "compression-supported",
  "document-format-default",
  "document-format-supported",
  "generated-natural-language-supported",
  "identify-actions-default",
  "identify-actions-supported",
  "ipp-features-supported",
  "ipp-versions-supported",
  "job-creation-attributes-supported",
  "job-ids-supported",
  "multiple-document-jobs-supported",
  "multiple-operation-time-out",
  "multiple-operation-time-out-action",
  "natural-language-configured",
  "operations-supported",
  "pages-per-minute",
  "pdl-override-supported",
  "preferred-attributes-supported",
  "printer-config-change-date-time",
  "printer-config-change-time",
  "printer-current-time",
  "printer-geo-location",
  "printer-get-attributes-supported",
  "printer-icons",
  "printer-info",
  "printer-location",
  "printer-make-and-model",
  "printer-more-info",
  "printer-organization",
  "printer-organizational-unit",
  "printer-state-change-date-time",
  "printer-state-change-time",
  "printer-state-message",
  "printer-uuid",
  "printer-up-time",
  "printer-name",
  "printer-is-accepting-jobs",
  "printer-state",
  "printer-state-reasons",
  "printer-up-time",
  "printer-uri-supported",
  "pwg-raster-document-resolution-supported",
  "pwg-raster-document-type-supported",
  "queued-job-count",
  "uri-authentication-supported",
  "uri-security-supported",
  "which-jobs-supported"
};

IppStream::IppStream(WiFiClient conn): HttpStream(conn) {
}

IppStream::~IppStream() {
}

std::map<String, std::set<String>> IppStream::parseRequestAttributes() {
  std::map<String, std::set<String>> result;
  byte tag = read();
  if (tag == IPP_OPERATION_ATTRIBUTES_TAG) {
    String name = "";
    int index = 0;
    while ((tag = read()) >= 0x10) { //if tag >= 0x10, then it's a value-tag
      uint16_t nameLength = read2Bytes();
      if (nameLength != 0) {
        name = readString(nameLength);
      } //otherwise, it's another value for the previous attribute
      uint16_t valueLength = read2Bytes();
      String value = readString(valueLength);
      if ((index == 0 && name != "attributes-charset") || (index == 1 && name != "attributes-natural-language")) {
        result.clear();
        return result;
      }
      result[name].insert(value);
      index++;
      Serial.printf("Parsed IPP attribute: tag=0x%02X, name=\"%s\", value=\"%s\"\r\n", tag, name.c_str(), value.c_str());
    }
  }
  return result;
}

void IppStream::beginResponse(uint16_t statusCode, uint32_t requestId, String charset) {
  print("HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 OK\r\nContent-Type: application/ipp\r\n\r\n");
  write2Bytes(IPP_SUPPORTED_VERSION);
  write2Bytes(statusCode);
  write4Bytes(requestId);
  write(IPP_OPERATION_ATTRIBUTES_TAG);
  writeStringAttribute(IPP_VALUE_TAG_CHARSET, "attributes-charset", charset);
  writeStringAttribute(IPP_VALUE_TAG_NATURAL_LANGUAGE, "attributes-natural-language", "en-us");
}

void IppStream::writeStringAttribute(byte valueTag, String name, String value) {
  write(valueTag);
  write2Bytes((uint16_t) name.length());
  print(name);
  write2Bytes((uint16_t) value.length());
  print(value);
}

void IppStream::writeOOBAttribute(byte valueTag, String name) {
  write(valueTag);
  write2Bytes((uint16_t) name.length());
  print(name);
  write2Bytes(0);
}

void IppStream::writeByteAttribute(byte valueTag, String name, byte value) {
  write(valueTag);
  write2Bytes((uint16_t) name.length());
  print(name);
  write2Bytes(1);
  write(value);
}

void IppStream::write2BytesAttribute(byte valueTag, String name, uint16_t value) {
  write(valueTag);
  write2Bytes((uint16_t) name.length());
  print(name);
  write2Bytes(2);
  write2Bytes(value);
}

void IppStream::write4BytesAttribute(byte valueTag, String name, uint32_t value) {
  write(valueTag);
  write2Bytes((uint16_t) name.length());
  print(name);
  write2Bytes(4);
  write4Bytes(value);
}

void IppStream::writeNBytesAttribute(byte valueTag, String name, uint16_t n, const char *value) {
  write(valueTag);
  write2Bytes((uint16_t) name.length());
  print(name);
  write2Bytes(n);
  for (int i=0; i<n; i++) {
    write(*value++);
  }
}

void IppStream::writeDateTimeAttribute(byte valueTag, String name, struct tm *t, long tz) {
  char rfc2579[11];
  rfc2579[0] = (char) ((t->tm_year + 1900) >> 8);
  rfc2579[1] = (char) ((t->tm_year + 1900) & 0xFF);
  rfc2579[2] = (char) (t->tm_mon) + 1;
  rfc2579[3] = (char) (t->tm_mday);
  rfc2579[4] = (char) (t->tm_hour);
  rfc2579[5] = (char) (t->tm_min);
  rfc2579[6] = (char) (t->tm_sec);
  rfc2579[7] = 0; // TODO deci-seconds
  rfc2579[8] = (tz > 0)? '+' : '-';
  rfc2579[9] = (char) (abs(tz) / 60);
  rfc2579[10] = (char) (abs(tz) % 60);
  Serial.printf("RFC2579 %d-%d-%d,%d:%d:%d.%d,%c%d:%d", 
    ntohs(*((uint16_t*) &rfc2579[0])),
    rfc2579[2],
    rfc2579[3],
    rfc2579[4],
    rfc2579[5],
    rfc2579[6],
    rfc2579[7],
    rfc2579[8],
    rfc2579[9],
    rfc2579[10]
  );
  writeNBytesAttribute(valueTag, name, 11, rfc2579);
}

void IppStream::writeResolutionAttribute(byte valueTag, String name, int32_t x, int32_t y, uint8_t unit) {
  write(valueTag);
  write2Bytes((uint16_t) name.length());
  print(name);
  write2Bytes(9);
  write4Bytes(x);
  write4Bytes(y);
  write(unit);
}

void IppStream::writePrinterAttribute(String name, Printer* printer) {
  if (name == "charset-configured") {
    writeStringAttribute(IPP_VALUE_TAG_CHARSET, name, "utf-8");
  } else if (name == "charset-supported") {
    writeStringAttribute(IPP_VALUE_TAG_CHARSET, name, "utf-8");
  } else if (name == "compression-supported") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "none");
  } else if (name == "copies-default") {
    write4BytesAttribute(IPP_VALUE_TAG_INTEGER, name, 1);
  } else if (name == "copies-supported") {
    write4BytesAttribute(IPP_VALUE_TAG_INTEGER, name, 1);
  } else if (name == "document-format-default") {
    writeStringAttribute(IPP_VALUE_TAG_MIME_MEDIA_TYPE, name, "text/plain");
  } else if (name == "document-format-supported") {
    writeStringAttribute(IPP_VALUE_TAG_MIME_MEDIA_TYPE, name, "text/plain");
    writeStringAttribute(IPP_VALUE_TAG_MIME_MEDIA_TYPE, "", "image/pwg-raster");
    writeStringAttribute(IPP_VALUE_TAG_MIME_MEDIA_TYPE, "", "image/jpeg");
  } else if (name == "generated-natural-language-supported") {
    writeStringAttribute(IPP_VALUE_TAG_NATURAL_LANGUAGE, name, "en-us");
  } else if (name == "identify-actions-default") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "flash"); // TODO Adjust according to identify printer
  } else if (name == "identify-actions-supported") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "flash"); // TODO Adjust according to identify printer
  } else if (name == "ipp-versions-supported") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "2.0");
  } else if (name == "ipp-features-supported") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "ipp-everywhere");
  } else if (name == "job-creation-attributes-supported") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "media");
  } else if (name == "job-ids-supported") {
    writeByteAttribute(IPP_VALUE_TAG_BOOLEAN, name, 1);
  } else if (name == "multiple-document-jobs-supported") {
    writeByteAttribute(IPP_VALUE_TAG_BOOLEAN, name, 0);
  } else if (name == "multiple-operation-time-out") {
    write4BytesAttribute(IPP_VALUE_TAG_INTEGER, name, 120);   // Timeout befor abort job
  } else if (name == "multiple-operation-time-out-action") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "abort-job");
  } else if (name == "natural-language-configured") {
    writeStringAttribute(IPP_VALUE_TAG_NATURAL_LANGUAGE, name, "en-us");
  } else if (name == "operations-supported") {
    write4BytesAttribute(IPP_VALUE_TAG_ENUM, name, IPP_PRINT_JOB);
    write4BytesAttribute(IPP_VALUE_TAG_ENUM, "", IPP_VALIDATE_JOB);
    write4BytesAttribute(IPP_VALUE_TAG_ENUM, "", IPP_CREATE_JOB);
    write4BytesAttribute(IPP_VALUE_TAG_ENUM, "", IPP_SEND_DOCUMENT);
    write4BytesAttribute(IPP_VALUE_TAG_ENUM, "", IPP_CANCEL_JOB);
    write4BytesAttribute(IPP_VALUE_TAG_ENUM, "", IPP_GET_JOB_ATTRIBUTES);
    write4BytesAttribute(IPP_VALUE_TAG_ENUM, "", IPP_GET_JOBS);
    write4BytesAttribute(IPP_VALUE_TAG_ENUM, "", IPP_GET_PRINTER_ATTRIBUTES);
    write4BytesAttribute(IPP_VALUE_TAG_ENUM, "", IPP_CANCEL_MY_JOBS);
    write4BytesAttribute(IPP_VALUE_TAG_ENUM, "", IPP_CLOSE_JOB);
    write4BytesAttribute(IPP_VALUE_TAG_ENUM, "", IPP_IDENTIFY_PRINTER);
  } else if (name == "pages-per-minute") {
    write4BytesAttribute(IPP_VALUE_TAG_INTEGER, name, 1);               // TODO correct this value
  } else if (name == "pdl-override-supported") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "not-attempted");
/*  } else if (name == "printer-up-time") {
    writeStringAttribute(IPP_VALUE_TAG_ */
  } else if (name == "preferred-attributes-supported") {
    writeByteAttribute(IPP_VALUE_TAG_BOOLEAN, name, 0);
  } else if (name == "printer-config-change-date-time") {
    writeNBytesAttribute(IPP_VALUE_TAG_DATETIME, name, 11, "\x07\xe5\x05\x1a\x13\x00\x32\x3+\x02\x00"); // 2021-05-26 19:00:50.3+0200
  } else if (name == "printer-config-change-time") {
    write4BytesAttribute(IPP_VALUE_TAG_INTEGER, name, 1622048450); // 2021-05-26 19:00:50+0200 in unix epoch
  } else if (name == "printer-current-time") {
    time_t now;
    time(&now);
    writeDateTimeAttribute(IPP_VALUE_TAG_DATETIME, name, localtime(&now), _timezone);
  } else if (name == "printer-geo-location") {
    writeOOBAttribute(IPP_VALUE_TAG_UNKNOWN, name);
  } else if (name == "printer-get-attributes-supported") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "document-format");
  } else if (name == "printer-icons") {
    writeStringAttribute(IPP_VALUE_TAG_URI, name, "http://Inkplate6.local:" + String(IPP_SERVER_PORT) + "/icon.png"); // TODO Dynamic name
  } else if (name == "printer-info") {
    writeStringAttribute(IPP_VALUE_TAG_TEXT, name, "This is a 6 inch one-page e-ink printer");
  } else if (name == "printer-location") {
    writeStringAttribute(IPP_VALUE_TAG_TEXT, name, "The kitchen");
  } else if (name == "printer-make-and-model") {
    writeStringAttribute(IPP_VALUE_TAG_TEXT, name, "e-radonica.com Inkplate6");
  } else if (name == "printer-more-info") {
    writeStringAttribute(IPP_VALUE_TAG_URI, name, "http://Inkplate6.local:" + String(IPP_SERVER_PORT) + "/info.html");
  } else if (name == "printer-organization") {
    writeStringAttribute(IPP_VALUE_TAG_TEXT, name, "The World");
  } else if (name == "printer-organizational-unit") {
    writeStringAttribute(IPP_VALUE_TAG_TEXT, name, "Everyone");
  } else if (name == "printer-state-change-date-time") {
    time_t now;
    time(&now);
    writeDateTimeAttribute(IPP_VALUE_TAG_DATETIME, name, localtime(&now), _timezone);
  } else if (name == "printer-state-change-time") {
    time_t now;
    time(&now);
    write4BytesAttribute(IPP_VALUE_TAG_INTEGER, name, now);
  } else if (name == "printer-state-message") {
    writeStringAttribute(IPP_VALUE_TAG_TEXT, name, "Ready"); // TODO Are we ready, really?
  } else if (name == "printer-uuid") {
    writeStringAttribute(IPP_VALUE_TAG_URI, name, "urn:uuid:FCE0B1BE-65B2-41D8-98E2-F020D6831DF7");
  } else if (name == "printer-name") {
    writeStringAttribute(IPP_VALUE_TAG_NAME, name, printer->getName());
  } else if (name =="printer-is-accepting-jobs") {
    writeByteAttribute(IPP_VALUE_TAG_BOOLEAN, name, 1);
  } else if (name == "printer-state") {
    write4BytesAttribute(IPP_VALUE_TAG_ENUM, name, 3); //3 = idle
  } else if (name == "printer-state-reasons") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "none");
  } else if (name == "printer-up-time") {
    write4BytesAttribute(IPP_VALUE_TAG_INTEGER, name, millis() / 1000);
  } else if (name == "printer-uri-supported") {
    writeStringAttribute(IPP_VALUE_TAG_URI, name, "ipp://Inkplate6.local:" + String(IPP_SERVER_PORT) + "/" + printer->getName()); // TODO String format with WiFi.localIP()
  } else if (name == "pwg-raster-document-resolution-supported") {
    writeResolutionAttribute(IPP_VALUE_TAG_RESOLUTION, name, 167, 167, 3); // 167 x 167 dots per inch
  } else if (name == "pwg-raster-document-type-supported") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "black_1");
  }else if (name == "queued-job-count") {
    write4BytesAttribute(IPP_VALUE_TAG_INTEGER, name, 0);
  } else if (name == "uri-authentication-supported") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "none");
  } else if (name == "uri-security-supported") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "none");
  } else if (name == "color-supported") {
    writeByteAttribute(IPP_VALUE_TAG_BOOLEAN, name, 0);
  } else if (name == "which-jobs-supported") {
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, name, "completed");
    writeStringAttribute(IPP_VALUE_TAG_KEYWORD, "", "not-completed");
  }
}

void IppStream::handleGetPrinterAttributesRequest(std::map<String, std::set<String>> requestAttributes, Printer* printer) {
  std::set<String>& requestedAttributes = requestAttributes["requested-attributes"];

  if (requestedAttributes.size() == 0 || (requestedAttributes.find("all") != requestedAttributes.end()) || (requestedAttributes.find("printer-description") != requestedAttributes.end())) {
    requestedAttributes = allPrinterDescriptionAttributes;
  }

  write(IPP_PRINTER_ATTRIBUTES_TAG);
  for (String attributeName: requestedAttributes) {
    writePrinterAttribute(attributeName, printer);
  }

  write(IPP_END_OF_ATTRIBUTES_TAG);
}

int IppStream::parseRequest(Printer** printers, int printerCount) {
  if (!parseRequestHeader()) {
    return -1;
  }

  if (getRequestMethod() == "GET") {
    if (getRequestPath() == "icon.png") {
      print("HTTP/1.1 200 OK\r\n\r\n");
      print("Content-Type: image/png\r\n");
      print("Content-Length: " + String(inkplate6_png_len) + "\r\n");
      print("\r\n");
      for (int i = 0; i< inkplate6_png_len; i++) {
        write(inkplate6_png[i]);
      }
      return -1;
    } else if (getRequestPath() == "info.html") {
      print("HTTP/1.1 301 Moved permanently\r\n");
      print("Location: https://e-radionica.com/en/inkplate-6.html\r\n");
      return -1;
    }
  }

  if (getRequestMethod() != "POST") {
    print("HTTP/1.1 405 Method Not Allowed\r\n");
    return -1;
  }

  Printer* printer = NULL;
  int printerIndex = -1;
  for (int i = 0; i < printerCount; i++) {
    if (("/" + printers[i]->getName()) == getRequestPath()) {
      printer = printers[i];
      printerIndex = i;
      break;
    }
  }

  if (printer == NULL) {
    print("HTTP/1.1 404 Not Found\r\n");
    return -1;
  }

  uint16_t ippVersion = read2Bytes();
  uint16_t operationId = read2Bytes();
  uint32_t requestId = read4Bytes();
  Serial.printf("Received IPP request; Version: 0x%04X, OperationId: 0x%04X, RequestId: 0x%08X\r\n", ippVersion, operationId, requestId);

  if (ippVersion != IPP_SUPPORTED_VERSION) {
    Serial.println("Unsupported IPP version");
    beginResponse(IPP_SERVER_ERROR_VERSION_NOT_SUPPORTED, requestId, "utf-8");
    write(IPP_END_OF_ATTRIBUTES_TAG);
    return -1;
  }

  if (requestId == 0) { //request-id must not be 0 (RFC8011 Section 4.1.2)
    beginResponse(IPP_CLIENT_ERROR_BAD_REQUEST, requestId, "utf-8");
    write(IPP_END_OF_ATTRIBUTES_TAG);
    return -1;
  }

  std::map<String, std::set<String>> requestAttributes = parseRequestAttributes();

  if (requestAttributes.size() == 0) {
    beginResponse(IPP_CLIENT_ERROR_BAD_REQUEST, requestId, "utf-8");
    write(IPP_END_OF_ATTRIBUTES_TAG);
    return -1;
  }

  if (requestAttributes.count("printer-uri") == 0) {
    beginResponse(IPP_CLIENT_ERROR_BAD_REQUEST, requestId, "utf-8");
    write(IPP_END_OF_ATTRIBUTES_TAG);
    return -1;
  }
  
  switch (operationId) {
    case IPP_IDENTIFY_PRINTER:
      Serial.println("Operation is Identify-Printer");
      beginResponse(IPP_SUCCESFUL_OK, requestId, *requestAttributes["attributes-charset"].begin());
      //write(IPP_OPERATION_ATTRIBUTES_TAG);
      //writeStringAttribute(IPP_VALUE_TAG_KEYWORD, "status-message", "Message");
      write(IPP_END_OF_ATTRIBUTES_TAG);
      //flushSendBuffer();
      // TODO Flash a led or another print "Hello <Username>" on screen
      return -1;

    case IPP_GET_PRINTER_ATTRIBUTES:
      Serial.println("Operation is Get-printer-Attributes");
      beginResponse(IPP_SUCCESFUL_OK, requestId, *requestAttributes["attributes-charset"].begin());
      handleGetPrinterAttributesRequest(requestAttributes, printer);
      return -1;

    case IPP_PRINT_JOB:
      Serial.println("Operation is Print-Job");
      beginResponse(IPP_SUCCESFUL_OK, requestId, *requestAttributes["attributes-charset"].begin());
      write(IPP_JOB_ATTRIBUTES_TAG);
      write4BytesAttribute(IPP_VALUE_TAG_ENUM, "job-state", 5); //5 = processing
      writeStringAttribute(IPP_VALUE_TAG_KEYWORD, "job-state-reasons", "none");
      write(IPP_END_OF_ATTRIBUTES_TAG);
      flushSendBuffer();
      return printerIndex;

    case IPP_VALIDATE_JOB:
      Serial.println("Operation is Validate-Job");
      beginResponse(IPP_SUCCESFUL_OK, requestId, *requestAttributes["attributes-charset"].begin());
      write(IPP_END_OF_ATTRIBUTES_TAG);
      return -1;

    default:
      Serial.println("The requested operation is not supported!");
      beginResponse(IPP_SERVER_ERROR_OPERATION_NOT_SUPPORTED, requestId, "utf-8");
      write(IPP_END_OF_ATTRIBUTES_TAG);
      return -1;
  }
}
