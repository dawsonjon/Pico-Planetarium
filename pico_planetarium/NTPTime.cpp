//  _  ___  _   _____ _     _                 
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___ 
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/    
//
// Copyright (c) Jonathan P Dawson 2024
// filename: clock_drift.ino
// description:
//
// Get Time from NTP
//
//
// License: MIT
//

#include "NTPTime.h"


// NTP server details
static const char* ntpServerName = "pool.ntp.org";  // NTP server pool
static const int ntpPort = 123;                     // NTP server port (123 for NTP)

// Function to send an NTP request
void NTPTime :: sendNTPpacket(const char* ntpServer) {
  memset(udpBuffer, 0, udpBufferSize);
  udpBuffer[0] = 0b11100011;   // NTP request header

  udp.beginPacket(ntpServer, ntpPort);
  udp.write(udpBuffer, udpBufferSize);
  udp.endPacket();
}

// Function to fetch local time and internet time
void NTPTime :: setTime() {
    
    if (WiFi.status() != WL_CONNECTED){
      Serial.println("No Connection...");
      return;
    }

    int packetSize = udp.parsePacket();
    if (packetSize) {
      udp.read(udpBuffer, udpBufferSize);
      unsigned long highWord = word(udpBuffer[40], udpBuffer[41]);
      unsigned long lowWord = word(udpBuffer[42], udpBuffer[43]);
      time_t secsSince1900 = highWord << 16 | lowWord;
      highWord = word(udpBuffer[44], udpBuffer[45]);
      lowWord = word(udpBuffer[46], udpBuffer[47]);
      unsigned long fractionalSeconds = highWord << 16 | lowWord;

      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      time_t unix_time = secsSince1900 - seventyYears;

      //set clock to ntp time
      timeval tv = {.tv_sec = unix_time};
      settimeofday(&tv, NULL);
      Serial.println("Time Updated");
    }

    if(millis() - lastUpdated > 30000)
    {
      sendNTPpacket(ntpServerName);
      lastUpdated = millis();
    }
  }

void NTPTime :: begin()
{
  udp.begin(2390);  // Arbitrary local port for UDP communication
}
