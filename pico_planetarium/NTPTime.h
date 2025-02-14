//  _  ___  _   _____ _     _                 
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___ 
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/    
//
// Copyright (c) Jonathan P Dawson 2025
// filename: clock_drift.ino
// description:
//
// Get Time from NTP
//
//
// License: MIT
//


#ifndef _NTP_TIME_H_
#define _NTP_TIME_H_

#include <WiFi.h>
#include <WiFiUdp.h>
#include "Arduino.h"

class NTPTime
{

  WiFiUDP udp;
  static const int udpBufferSize = 48;
  byte udpBuffer[udpBufferSize];
  uint64_t lastUpdated = 0;
  void sendNTPpacket(const char* ntpServer);

  public:

  void setTime();
  void begin();

};

#endif
