#include "Arduino.h"

/*
 * This sketch will query the network for hosts providing the service defined by
 * QUESTION_SERVICE and will parse any replies with the aim of saving port and
 * network address in the hosts array.
 */


#include "MDNSClient.h"

#include "secrets.h"  // Contains the following:
// char ssid[] = "Get off my wlan";      //  your network SSID (name)
// char pass[] = "secretwlanpass";       // your network password
//#define QUESTION_HOST "myhostname.local"

int status = WL_IDLE_STATUS;        // Indicator of WiFi status

void printWifiData();
void printCurrentNet();

#define QUESTION_SERVICE "_mqtt._tcp.local"


// Make this value as large as available ram allows.
#define MAX_MDNS_PACKET_SIZE 512

WiFiUDP udp;

// buffer can be used bu other processes that need a large chunk of memory.
byte buffer[MAX_MDNS_PACKET_SIZE];
mdns::MDns my_mdns(udp, buffer, MAX_MDNS_PACKET_SIZE);
MDNSClient mdnsClient(my_mdns);

void setup()
{
    //Initialize serial and wait for port to open:
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    // attempt to connect to Wifi network:
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to WPA SSID: ");
        status = WiFi.begin(ssid, pass);
        delay(1000);
    }

    // you're connected now, so print out the data:
    Serial.println();
    Serial.print("You're connected to the network");
    printCurrentNet();
    printWifiData();

    my_mdns.begin(); // call to startUdpMulticast
}

unsigned int last_packet_count = 0;
unsigned long timer1 = millis();
bool requestSrv = true;
void loop()
{
	if (millis() - timer1 >= 10000)
	{
		timer1 = millis();

		if (requestSrv)
		{
			int hostCount = mdnsClient.lookupService(QUESTION_SERVICE);
			Serial.print(QUESTION_SERVICE "=====> resolved to ");
			Serial.print(hostCount);
			Serial.println(" hosts");
		}
		else
		{
			IPAddress host = mdnsClient.lookupHost(QUESTION_HOST);
			Serial.print(QUESTION_HOST "=====> resolved to: ");
			Serial.println(host);
		}

		requestSrv = !requestSrv;
	}

  my_mdns.loop();

#ifdef DEBUG_STATISTICS
  // Give feedback on the percentage of incoming mDNS packets that fitted in buffer.
  // Useful for tuning the buffer size to make best use of available memory.
  if(last_packet_count != my_mdns.packet_count && my_mdns.packet_count != 0){
    last_packet_count = my_mdns.packet_count;
    Serial.print("mDNS decode success rate: ");
    Serial.print(100 - (100 * my_mdns.buffer_size_fail / my_mdns.packet_count));
    Serial.print("%\nLargest packet size: ");
    Serial.println(my_mdns.largest_packet_seen);
  }
#endif

  // mDNS not using buffer outside my_mdns.loop() so it can be used for other tasks.
  strncpy((char*)buffer,
          "<html><head>Some webpage that needs a large buffer</head>"
          "<body>big content...</body></html>",
          MAX_MDNS_PACKET_SIZE);
  // Display buffer here....
}

void printWifiData() {
    // print your WiFi IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
    Serial.println(ip);

    // print your MAC address:
    byte mac[6];
    WiFi.macAddress(mac);
    Serial.print("MAC address: ");
    Serial.print(mac[0], HEX);
    Serial.print(":");
    Serial.print(mac[1], HEX);
    Serial.print(":");
    Serial.print(mac[2], HEX);
    Serial.print(":");
    Serial.print(mac[3], HEX);
    Serial.print(":");
    Serial.print(mac[4], HEX);
    Serial.print(":");
    Serial.println(mac[5], HEX);
}

void printCurrentNet() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print the MAC address of the router you're attached to:
    byte bssid[6];
    WiFi.BSSID(bssid);
    Serial.print("BSSID: ");
    Serial.print(bssid[5], HEX);
    Serial.print(":");
    Serial.print(bssid[4], HEX);
    Serial.print(":");
    Serial.print(bssid[3], HEX);
    Serial.print(":");
    Serial.print(bssid[2], HEX);
    Serial.print(":");
    Serial.print(bssid[1], HEX);
    Serial.print(":");
    Serial.println(bssid[0], HEX);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.println(rssi);

    // print the encryption type:
    byte encryption = WiFi.encryptionType();
    Serial.print("Encryption Type:");
    Serial.println(encryption, HEX);
    Serial.println();
}
