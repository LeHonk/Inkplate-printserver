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
#include <Arduino.h>
#include <WiFiClient.h>
#include <map>
#include <set>
#include "HttpStream.h"
#include "Printer.h"

#define IPP_SUPPORTED_VERSION 0x0200

#define IPP_SUCCESFUL_OK 0x0000
#define IPP_CLIENT_ERROR_BAD_REQUEST 0x0400
#define IPP_SERVER_ERROR_OPERATION_NOT_SUPPORTED 0x0501
#define IPP_SERVER_ERROR_VERSION_NOT_SUPPORTED 0x0503

#define IPP_OPERATION_ATTRIBUTES_TAG 0x01
#define IPP_JOB_ATTRIBUTES_TAG 0x02
#define IPP_END_OF_ATTRIBUTES_TAG 0x03
#define IPP_PRINTER_ATTRIBUTES_TAG 0x04
#define IPP_UNSUPPORTED_ATTRIBUTES_TAG 0x05

#define IPP_VALUE_TAG_UNSUPPORTED 0x10
#define IPP_VALUE_TAG_UNKNOWN 0x12
#define IPP_VALUE_TAG_NOVALUE 0x13
#define IPP_VALUE_TAG_INTEGER 0x21
#define IPP_VALUE_TAG_BOOLEAN 0x22
#define IPP_VALUE_TAG_ENUM 0x23
#define IPP_VALUE_TAG_DATETIME 0x31
#define IPP_VALUE_TAG_RESOLUTION 0x32
#define IPP_VALUE_TAG_TEXT 0x41
#define IPP_VALUE_TAG_NAME 0x42
#define IPP_VALUE_TAG_KEYWORD 0x44
#define IPP_VALUE_TAG_URI 0x45
#define IPP_VALUE_TAG_CHARSET 0x47
#define IPP_VALUE_TAG_NATURAL_LANGUAGE 0x48
#define IPP_VALUE_TAG_MIME_MEDIA_TYPE 0x49

// IPP Operations
#define IPP_PRINT_JOB               0x0002
#define IPP_VALIDATE_JOB            0x0004
#define IPP_CREATE_JOB              0x0005
#define IPP_SEND_DOCUMENT           0x0006
#define IPP_CANCEL_JOB              0x0008
#define IPP_GET_JOB_ATTRIBUTES      0x0009
#define IPP_GET_JOBS                0x000A
#define IPP_GET_PRINTER_ATTRIBUTES  0x000B
#define IPP_CANCEL_MY_JOBS          0x0039
#define IPP_CLOSE_JOB               0x003B
#define IPP_IDENTIFY_PRINTER        0x003C

class IppStream: public HttpStream {
  private:
    std::map<String, std::set<String>> parseRequestAttributes();
    void beginResponse(uint16_t statusCode, uint32_t requestId, String charset);

    void writeStringAttribute(byte valueTag, String name, String value);
    void writeOOBAttribute(byte valueTag, String name);
    void writeByteAttribute(byte valueTag, String name, byte value);
    void write2BytesAttribute(byte valueTag, String name, uint16_t value);
    void write4BytesAttribute(byte valueTag, String name, uint32_t value);
    void writeNBytesAttribute(byte valueTag, String name, uint16_t n, const char *value);
    void writeDateTimeAttribute(byte valueTag, String name, struct tm* t, long tz);
    void writeResolutionAttribute(byte valueTag, String name, int32_t x, int32_t y, uint8_t unit);

    void writePrinterAttribute(String name, Printer* printer);
    void handleGetPrinterAttributesRequest(std::map<String, std::set<String>> requestAttributes, Printer* printer);

  public:
    IppStream(WiFiClient conn);
    virtual~IppStream(); 
    int parseRequest(Printer** printers, int printerCount);
};
