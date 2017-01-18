// Code from : https://github.com/bportaluri/WiFiEsp/blob/master/examples/UdpNTPClient/UdpNTPClient.ino

#include "WiFiEspUdp.h"

char timeServer[] = "time.nist.gov";  // NTP server
unsigned int localPort = 2390;        // local port to listen for UDP packets

const int NTP_PACKET_SIZE = 48;  // NTP timestamp is in the first 48 bytes of the message
const int UDP_TIMEOUT = 2000;    // timeout in miliseconds to wait for an UDP packet to arrive

byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets
WiFiEspUDP Udp;

void ntpSync(void) {
	Udp.begin(localPort);

	while (1) {
		sendNTPpacket(timeServer); // send an NTP packet to a time server

		// wait for a reply for UDP_TIMEOUT miliseconds
		unsigned long startMs = millis();
		while (!Udp.available() && (millis() - startMs) < UDP_TIMEOUT) {}

		Serial.println(Udp.parsePacket());
		if (Udp.parsePacket()) {
			Serial.println("packet received");
			// We've received a packet, read the data from it into the buffer
			Udp.read(packetBuffer, NTP_PACKET_SIZE);

			// the timestamp starts at byte 40 of the received packet and is four bytes,
			// or two words, long. First, esxtract the two words:

			unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
			unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
			// combine the four bytes (two words) into a long integer
			// this is NTP time (seconds since Jan 1 1900):
			unsigned long secsSince1900 = highWord << 16 | lowWord;
			Serial.print("Seconds since Jan 1 1900 = ");
			Serial.println(secsSince1900);

			// now convert NTP time into everyday time:
			Serial.print("Unix time = ");
			// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
			const unsigned long seventyYears = 2208988800UL;
			// subtract seventy years:
			unsigned long epoch = secsSince1900 - seventyYears;
			// print Unix time:
			Serial.println(epoch);

			setTime(epoch);

			// print the hour, minute and second:
			Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
			Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
			Serial.print(':');
			if (((epoch % 3600) / 60) < 10) {
				// In the first 10 minutes of each hour, we'll want a leading '0'
				Serial.print('0');
			}
			Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
			Serial.print(':');
			if ((epoch % 60) < 10) {
				// In the first 10 seconds of each minute, we'll want a leading '0'
				Serial.print('0');
			}
			Serial.println(epoch % 60); // print the second
			break;
		}
		// wait ten seconds before asking for the time again
		delay(10000);
	}
}

// send an NTP request to the time server at the given address
void sendNTPpacket(char *ntpSrv)
{
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)

	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12]  = 49;
	packetBuffer[13]  = 0x4E;
	packetBuffer[14]  = 49;
	packetBuffer[15]  = 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	Udp.beginPacket(ntpSrv, 123); //NTP requests are to port 123

	Udp.write(packetBuffer, NTP_PACKET_SIZE);

	Udp.endPacket();
}
